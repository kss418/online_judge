#include "judge_server/judge_worker.hpp"

#include "common/file_util.hpp"
#include "db_service/submission_service.hpp"
#include "judge_server/checker.hpp"
#include "judge_server/judge_util.hpp"
#include "judge_server/testcase_runner.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

std::expected<judge_worker, error_code> judge_worker::create(
    submission_event_listener submission_event_listener
){
    const auto source_directory_path_exp = judge_util::instance().make_source_directory_path();
    if(!source_directory_path_exp){
        return std::unexpected(source_directory_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        *source_directory_path_exp
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    auto listen_submission_queue_exp = submission_event_listener.listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    auto testcase_downloader_connection_exp = db_connection::create();
    if(!testcase_downloader_connection_exp){
        return std::unexpected(testcase_downloader_connection_exp.error());
    }

    auto testcase_downloader_exp = testcase_downloader::create(
        std::move(*testcase_downloader_connection_exp)
    );
    if(!testcase_downloader_exp){
        return std::unexpected(testcase_downloader_exp.error());
    }

    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    return judge_worker(
        std::move(submission_event_listener),
        std::move(*db_connection_exp),
        std::move(*testcase_downloader_exp)
    );
}

judge_worker::judge_worker(
    submission_event_listener submission_event_listener,
    db_connection db_connection,
    testcase_downloader testcase_downloader
) :
    submission_event_listener_(std::move(submission_event_listener)),
    db_connection_(std::move(db_connection)),
    testcase_downloader_(std::move(testcase_downloader)){}

submission_status judge_worker::to_submission_status(judge_result result){
    switch(result){
        case judge_result::accepted:
            return submission_status::accepted;
        case judge_result::wrong_answer:
            return submission_status::wrong_answer;
        case judge_result::time_limit_exceeded:
            return submission_status::time_limit_exceeded;
        case judge_result::memory_limit_exceeded:
            return submission_status::memory_limit_exceeded;
        case judge_result::runtime_error:
            return submission_status::runtime_error;
        case judge_result::compile_error:
            return submission_status::compile_error;
        case judge_result::output_exceeded:
            return submission_status::output_exceeded;
        case judge_result::invalid_output:
            return submission_status::wrong_answer;
    }

    return submission_status::wrong_answer;
}

judge_worker::finalize_submission_data judge_worker::make_finalize_submission_data(
    submission_status submission_status_value,
    const std::vector<sandbox_runner::run_result>& run_results
){
    finalize_submission_data finalize_submission_data_value;
    finalize_submission_data_value.score = std::int16_t{0};

    if(submission_status_value == submission_status::accepted){
        finalize_submission_data_value.score = std::int16_t{100};
    }

    if(
        submission_status_value != submission_status::compile_error &&
        !run_results.empty()
    ){
        std::int64_t max_elapsed_ms = 0;
        std::int64_t max_rss_kb = 0;

        for(const auto& run_result : run_results){
            max_elapsed_ms = std::max(
                max_elapsed_ms,
                static_cast<std::int64_t>(run_result.elapsed_ms_)
            );
            max_rss_kb = std::max(
                max_rss_kb,
                static_cast<std::int64_t>(run_result.max_rss_kb_)
            );
        }

        finalize_submission_data_value.elapsed_ms_opt = max_elapsed_ms;
        finalize_submission_data_value.max_rss_kb_opt = max_rss_kb;
    }

    if(run_results.empty() || run_results.front().stderr_text_.empty()){
        return finalize_submission_data_value;
    }

    if(submission_status_value == submission_status::compile_error){
        finalize_submission_data_value.compile_output = run_results.front().stderr_text_;
        return finalize_submission_data_value;
    }

    if(
        submission_status_value == submission_status::runtime_error ||
        submission_status_value == submission_status::time_limit_exceeded ||
        submission_status_value == submission_status::memory_limit_exceeded
    ){
        finalize_submission_data_value.judge_output = run_results.front().stderr_text_;
    }

    return finalize_submission_data_value;
}

std::expected<judge_result, error_code> judge_worker::check_result(
    std::int64_t problem_id,
    const testcase_runner::run_batch& run_batch_value
){
    std::vector<std::vector<std::string>> output_lines;
    output_lines.reserve(run_batch_value.run_results.size());

    if(run_batch_value.compile_failed){
        return judge_result::compile_error;
    }

    for(const auto& run_result : run_batch_value.run_results){
        if(run_result.time_limit_exceeded_){
            return judge_result::time_limit_exceeded;
        }
        if(run_result.memory_limit_exceeded_){
            return judge_result::memory_limit_exceeded;
        }
        if(run_result.exit_code_ != 0){
            return judge_result::runtime_error;
        }

        output_lines.push_back(run_result.output_lines_);
    }

    return checker::check_all(output_lines, problem_id);
}

std::expected<void, error_code> judge_worker::run(){
    while(true){
        auto queued_submission_opt_exp = lease_submission();
        if(!queued_submission_opt_exp){
            return std::unexpected(queued_submission_opt_exp.error());
        }

        if(!queued_submission_opt_exp->has_value()){
            auto wait_submission_notification_exp =
                submission_event_listener_.wait_submission_notification(
                    NOTIFICATION_WAIT_TIMEOUT
                );
            if(!wait_submission_notification_exp){
                return std::unexpected(wait_submission_notification_exp.error());
            }
            continue;
        }

        const submission_dto::queued_submission& queued_submission_value =
            queued_submission_opt_exp->value();
        const auto process_submission_exp = process_submission(
            queued_submission_value
        );
        
        if(!process_submission_exp){
            const auto mark_queued_exp = submission_service::mark_queued(
                db_connection_,
                queued_submission_value.submission_id
            );
            if(!mark_queued_exp){
                return std::unexpected(mark_queued_exp.error());
            }
            continue;
        }
    }
}

std::expected<void, error_code> judge_worker::process_submission(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto source_file_path_exp = prepare_submission(queued_submission_value);
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    const auto mark_judging_exp = submission_service::mark_judging(
        db_connection_,
        queued_submission_value.submission_id
    );
    if(!mark_judging_exp){
        return std::unexpected(mark_judging_exp.error());
    }

    auto judge_submission_exp = judge_submission(
        *source_file_path_exp,
        queued_submission_value.problem_id
    );
    if(!judge_submission_exp){
        return std::unexpected(judge_submission_exp.error());
    }

    const auto finalize_submission_exp = finalize_submission(
        queued_submission_value.submission_id,
        judge_submission_exp->judge_result_value,
        judge_submission_exp->run_results
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<judge_worker::process_submission_data, error_code> judge_worker::judge_submission(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id
){
    auto run_all_testcases_exp = testcase_runner::run_all_testcases(
        source_file_path,
        problem_id
    );
    if(!run_all_testcases_exp){
        return std::unexpected(run_all_testcases_exp.error());
    }

    const auto judge_result_exp = check_result(
        problem_id,
        *run_all_testcases_exp
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    process_submission_data process_submission_data_value;
    process_submission_data_value.judge_result_value = *judge_result_exp;
    process_submission_data_value.run_results = std::move(run_all_testcases_exp->run_results);
    return process_submission_data_value;
}

std::expected<void, error_code> judge_worker::finalize_submission(
    std::int64_t submission_id,
    judge_result result,
    const std::vector<sandbox_runner::run_result>& run_results
){
    const submission_status submission_status_value = to_submission_status(result);
    const finalize_submission_data finalize_submission_data_value =
        make_finalize_submission_data(
            submission_status_value,
            run_results
        );
    const submission_dto::finalize_request finalize_request_value =
        submission_dto::make_finalize_request(
            submission_id,
            submission_status_value,
            finalize_submission_data_value.score,
            finalize_submission_data_value.compile_output,
            finalize_submission_data_value.judge_output,
            finalize_submission_data_value.elapsed_ms_opt,
            finalize_submission_data_value.max_rss_kb_opt
        );

    const auto finalize_submission_exp = submission_service::finalize_submission(
        db_connection_,
        finalize_request_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}
std::expected<std::filesystem::path, error_code> judge_worker::prepare_submission(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto source_file_path_exp = judge_util::instance().make_source_file_path(
        queued_submission_value.submission_id,
        queued_submission_value.language
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    auto create_file_exp = file_util::create_file(
        *source_file_path_exp,
        queued_submission_value.source_code
    );
    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    const auto sync_testcases_exp = testcase_downloader_.sync_testcases(
        queued_submission_value.problem_id
    );
    if(!sync_testcases_exp){
        return std::unexpected(sync_testcases_exp.error());
    }

    return *source_file_path_exp;
}

std::expected<std::optional<submission_dto::queued_submission>, error_code> judge_worker::lease_submission(){
    submission_dto::lease_request lease_request_value;
    lease_request_value.lease_duration = LEASE_DURATION;
    return submission_service::lease_submission(
        db_connection_,
        lease_request_value
    );
}
