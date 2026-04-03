#include "judge_core/judge_worker.hpp"

#include "common/file_util.hpp"
#include "common/logger.hpp"
#include "common/timer.hpp"
#include "db_service/submission_service.hpp"
#include "judge_core/judge_service.hpp"
#include "judge_core/checker.hpp"
#include "judge_core/judge_util.hpp"
#include "judge_core/testcase_runner.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace{
    void log_submission_stage_metrics(
        std::int64_t submission_id,
        const judge_worker::submission_stage_metrics& submission_stage_metrics_value
    ){
        logger::clog()
            .log("submission_stage_metrics")
            .field("thread_id", std::this_thread::get_id())
            .field("submission_id", submission_id)
            .field("event", submission_stage_metrics_value.event)
            .field(
                "final_status",
                to_string(submission_stage_metrics_value.final_submission_status)
            )
            .field("queue_wait_ms", submission_stage_metrics_value.queue_wait_ms)
            .field(
                "prepare_workspace_ms",
                submission_stage_metrics_value.prepare_workspace_elapsed_ms
            )
            .field(
                "testcase_snapshot_ms",
                submission_stage_metrics_value.testcase_snapshot_elapsed_ms
            )
            .field(
                "compile_prepare_ms",
                submission_stage_metrics_value.compile_prepare_elapsed_ms
            )
            .field(
                "testcase_run_ms",
                submission_stage_metrics_value.testcase_execution_elapsed_ms
            )
            .field("finalize_ms", submission_stage_metrics_value.finalize_elapsed_ms)
            .field("cleanup_ms", submission_stage_metrics_value.cleanup_elapsed_ms)
            .field("total_ms", submission_stage_metrics_value.total_elapsed_ms)
            .field("testcase_count", submission_stage_metrics_value.testcase_count)
            .optional_field("error", submission_stage_metrics_value.error_message_opt);
    }
}

std::expected<judge_worker, judge_error> judge_worker::create(
    submission_event_listener submission_event_listener,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
){
    if(!problem_lock_registry){
        return std::unexpected(
            judge_error{
                judge_error_code::internal,
                "problem lock registry is not configured"
            }
        );
    }

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

    auto db_config_exp = db_connection::load_db_connection_config();
    if(!db_config_exp){
        return std::unexpected(db_config_exp.error());
    }

    auto testcase_downloader_connection_exp = db_connection::create(*db_config_exp);
    if(!testcase_downloader_connection_exp){
        return std::unexpected(testcase_downloader_connection_exp.error());
    }

    auto testcase_downloader_exp = testcase_downloader::create(
        std::move(*testcase_downloader_connection_exp)
    );
    if(!testcase_downloader_exp){
        return std::unexpected(testcase_downloader_exp.error());
    }

    auto db_connection_exp = db_connection::create(*db_config_exp);
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    return judge_worker(
        std::move(submission_event_listener),
        std::move(*db_connection_exp),
        std::move(*testcase_downloader_exp),
        std::move(problem_lock_registry)
    );
}

judge_worker::judge_worker(
    submission_event_listener submission_event_listener,
    db_connection db_connection,
    testcase_downloader testcase_downloader,
    std::shared_ptr<problem_lock_registry> problem_lock_registry
) :
    submission_event_listener_(std::move(submission_event_listener)),
    db_connection_(std::move(db_connection)),
    testcase_downloader_(std::move(testcase_downloader)),
    problem_lock_registry_(std::move(problem_lock_registry)){}

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
        submission_status_value == submission_status::memory_limit_exceeded ||
        submission_status_value == submission_status::output_exceeded
    ){
        finalize_submission_data_value.judge_output = run_results.front().stderr_text_;
    }

    return finalize_submission_data_value;
}

judge_worker::submission_stage_metrics judge_worker::make_submission_stage_metrics(
    const submission_dto::queued_submission& queued_submission_value
){
    submission_stage_metrics submission_stage_metrics_value;
    submission_stage_metrics_value.queue_wait_ms = queued_submission_value.queue_wait_ms;
    return submission_stage_metrics_value;
}

judge_worker::submission_stage_metrics judge_worker::make_submission_stage_metrics(
    submission_stage_metrics submission_stage_metrics_value,
    const process_submission_data& process_submission_data_value
){
    submission_stage_metrics_value.testcase_count =
        process_submission_data_value.testcase_count;
    submission_stage_metrics_value.compile_prepare_elapsed_ms =
        process_submission_data_value.compile_prepare_elapsed_ms;
    submission_stage_metrics_value.testcase_execution_elapsed_ms =
        process_submission_data_value.testcase_execution_elapsed_ms;
    submission_stage_metrics_value.final_submission_status = to_submission_status(
        process_submission_data_value.judge_result_value
    );
    return submission_stage_metrics_value;
}

judge_worker::submission_stage_metrics judge_worker::make_requeued_submission_stage_metrics(
    const submission_dto::queued_submission& queued_submission_value,
    std::string error_message
){
    submission_stage_metrics submission_stage_metrics_value = make_submission_stage_metrics(
        queued_submission_value
    );
    submission_stage_metrics_value.event = "requeued";
    submission_stage_metrics_value.error_message_opt = std::move(error_message);
    return submission_stage_metrics_value;
}

judge_worker::process_submission_data judge_worker::make_process_submission_data(
    judge_result judge_result_value,
    testcase_runner::run_batch&& run_batch_value
){
    process_submission_data process_submission_data_value;
    process_submission_data_value.judge_result_value = judge_result_value;
    process_submission_data_value.testcase_count = run_batch_value.testcase_count;
    process_submission_data_value.compile_prepare_elapsed_ms =
        run_batch_value.prepare_elapsed_ms;
    process_submission_data_value.testcase_execution_elapsed_ms =
        run_batch_value.testcase_execution_elapsed_ms;
    process_submission_data_value.run_results = std::move(run_batch_value.run_results);
    return process_submission_data_value;
}

std::expected<judge_result, judge_error> judge_worker::check_result(
    const testcase_snapshot& testcase_snapshot_value,
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
        if(run_result.output_exceeded_){
            return judge_result::output_exceeded;
        }
        if(run_result.memory_limit_exceeded_){
            return judge_result::memory_limit_exceeded;
        }
        if(run_result.exit_code_ != 0){
            return judge_result::runtime_error;
        }

        output_lines.push_back(run_result.output_lines_);
    }

    return checker::check_all(output_lines, testcase_snapshot_value);
}

std::expected<void, judge_error> judge_worker::run(){
    while(true){
        auto queued_submission_opt_exp = judge_service::poll_next_submission(
            db_connection_,
            submission_event_listener_,
            LEASE_DURATION,
            NOTIFICATION_WAIT_TIMEOUT
        );
        if(!queued_submission_opt_exp){
            return std::unexpected(queued_submission_opt_exp.error());
        }

        if(!queued_submission_opt_exp->has_value()){
            continue;
        }

        const submission_dto::queued_submission& queued_submission_value =
            queued_submission_opt_exp->value();
        timer processing_timer;
        const auto process_submission_exp = process_submission(
            queued_submission_value
        );

        submission_stage_metrics submission_stage_metrics_value;
        if(process_submission_exp){
            submission_stage_metrics_value = *process_submission_exp;
        }
        else{
            submission_stage_metrics_value = make_requeued_submission_stage_metrics(
                queued_submission_value,
                to_string(process_submission_exp.error())
            );
        }

        const auto cleanup_workspace_exp = timer::measure_elapsed_ms(
            submission_stage_metrics_value.cleanup_elapsed_ms,
            [this, &queued_submission_value]{
                return cleanup_submission_workspace(
                    queued_submission_value.submission_id
                );
            }
        );
        if(!cleanup_workspace_exp){
            return std::unexpected(cleanup_workspace_exp.error());
        }

        if(!process_submission_exp){
            const auto requeue_submission_exp = requeue_submission(
                queued_submission_value.submission_id,
                *submission_stage_metrics_value.error_message_opt
            );
            if(!requeue_submission_exp){
                return std::unexpected(requeue_submission_exp.error());
            }
        }

        submission_stage_metrics_value.total_elapsed_ms = processing_timer.elapsed_ms();
        log_submission_stage_metrics(
            queued_submission_value.submission_id,
            submission_stage_metrics_value
        );
    }
}

std::expected<judge_worker::submission_stage_metrics, judge_error>
judge_worker::process_submission(
    const submission_dto::queued_submission& queued_submission_value
){
    submission_stage_metrics submission_stage_metrics_value = make_submission_stage_metrics(
        queued_submission_value
    );

    const auto source_file_path_exp = timer::measure_elapsed_ms(
        submission_stage_metrics_value.prepare_workspace_elapsed_ms,
        [this, &queued_submission_value]{
            return prepare_submission(queued_submission_value);
        }
    );
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

    const auto testcase_snapshot_exp = timer::measure_elapsed_ms(
        submission_stage_metrics_value.testcase_snapshot_elapsed_ms,
        [this, &queued_submission_value]() -> std::expected<testcase_snapshot, judge_error> {
            auto problem_lock_exp = problem_lock_registry_->lock(
                queued_submission_value.problem_id
            );
            if(!problem_lock_exp){
                return std::unexpected(problem_lock_exp.error());
            }

            return testcase_downloader_.ensure_testcase_snapshot(
                queued_submission_value.problem_id
            );
        }
    );
    if(!testcase_snapshot_exp){
        return std::unexpected(testcase_snapshot_exp.error());
    }

    auto judge_submission_exp = judge_submission(
        *source_file_path_exp,
        *testcase_snapshot_exp
    );
    if(!judge_submission_exp){
        return std::unexpected(judge_submission_exp.error());
    }
    submission_stage_metrics_value = make_submission_stage_metrics(
        std::move(submission_stage_metrics_value),
        *judge_submission_exp
    );

    const auto finalize_submission_exp = timer::measure_elapsed_ms(
        submission_stage_metrics_value.finalize_elapsed_ms,
        [this, &queued_submission_value, &judge_submission_exp]{
            return finalize_submission(
                queued_submission_value.submission_id,
                judge_submission_exp->judge_result_value,
                judge_submission_exp->run_results
            );
        }
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return submission_stage_metrics_value;
}

std::expected<judge_worker::process_submission_data, judge_error>
judge_worker::judge_submission(
    const std::filesystem::path& source_file_path,
    const testcase_snapshot& testcase_snapshot_value
){
    auto run_all_testcases_exp = testcase_runner::run_all_testcases(
        source_file_path,
        testcase_snapshot_value
    );
    if(!run_all_testcases_exp){
        return std::unexpected(run_all_testcases_exp.error());
    }

    const auto judge_result_exp = check_result(
        testcase_snapshot_value,
        *run_all_testcases_exp
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    return make_process_submission_data(
        *judge_result_exp,
        std::move(*run_all_testcases_exp)
    );
}

std::expected<void, judge_error> judge_worker::finalize_submission(
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

std::expected<std::filesystem::path, judge_error> judge_worker::prepare_submission(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto workspace_path_exp = judge_util::instance().make_submission_workspace_path(
        queued_submission_value.submission_id
    );
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    const auto cleanup_workspace_exp = file_util::remove_all(*workspace_path_exp);
    if(!cleanup_workspace_exp){
        return std::unexpected(cleanup_workspace_exp.error());
    }

    const auto create_workspace_exp = file_util::create_directories(*workspace_path_exp);
    if(!create_workspace_exp){
        return std::unexpected(create_workspace_exp.error());
    }

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

    return *source_file_path_exp;
}

std::expected<void, judge_error> judge_worker::cleanup_submission_workspace(
    std::int64_t submission_id
){
    const auto workspace_path_exp = judge_util::instance().make_submission_workspace_path(
        submission_id
    );
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    return file_util::remove_all(*workspace_path_exp);
}

std::expected<void, judge_error> judge_worker::requeue_submission(
    std::int64_t submission_id,
    std::string reason
){
    return submission_service::requeue_submission_immediately(
        db_connection_,
        submission_id,
        std::move(reason)
    );
}
