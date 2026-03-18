#include "judge_server/judge_worker.hpp"

#include "common/file_util.hpp"
#include "db/submission_util.hpp"
#include "judge_server/checker.hpp"
#include "judge_server/judge_util.hpp"
#include "judge_server/testcase_runner.hpp"

#include <pqxx/pqxx>

#include <optional>
#include <string>
#include <utility>
#include <vector>

std::expected<judge_worker, error_code> judge_worker::create(submission_service submission_service){
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

    auto listen_submission_queue_exp = submission_service.listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    auto tc_downloader_connection_exp = db_connection::create();
    if(!tc_downloader_connection_exp){
        return std::unexpected(tc_downloader_connection_exp.error());
    }

    auto tc_downloader_exp = tc_downloader::create(
        std::move(*tc_downloader_connection_exp)
    );
    if(!tc_downloader_exp){
        return std::unexpected(tc_downloader_exp.error());
    }

    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    return judge_worker(
        std::move(submission_service),
        std::move(*db_connection_exp),
        std::move(*tc_downloader_exp)
    );
}

judge_worker::judge_worker(
    submission_service submission_service,
    db_connection db_connection,
    tc_downloader tc_downloader
) :
    submission_service_(std::move(submission_service)),
    db_connection_(std::move(db_connection)),
    tc_downloader_(std::move(tc_downloader)){}

bool judge_worker::is_queue_empty_error(const error_code& code){
    return code == errno_error::resource_temporarily_unavailable;
}

submission_status judge_worker::to_submission_status(judge_result result){
    switch(result){
        case judge_result::accepted:
            return submission_status::accepted;
        case judge_result::wrong_answer:
            return submission_status::wrong_answer;
        case judge_result::time_limit_excced:
            return submission_status::time_limit_exceeded;
        case judge_result::memory_limit_excced:
            return submission_status::memory_limit_exceeded;
        case judge_result::runtime_error:
            return submission_status::runtime_error;
        case judge_result::compile_error:
            return submission_status::compile_error;
        case judge_result::output_exceed:
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
        return finalize_submission_data_value;
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

std::expected<judge_result, error_code> judge_worker::judge_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const std::vector<sandbox_runner::run_result>& run_results
){
    std::vector<std::vector<std::string>> output_lines;
    output_lines.reserve(run_results.size());

    for(const auto& run_result : run_results){
        if(run_result.time_limit_exceeded_){
            return judge_result::time_limit_excced;
        }
        if(run_result.memory_limit_exceeded_){
            return judge_result::memory_limit_excced;
        }
        if(run_result.exit_code_ != 0){
            if(queued_submission_value.language == "cpp"){
                return judge_result::compile_error;
            }

            return judge_result::runtime_error;
        }

        output_lines.push_back(run_result.output_lines_);
    }

    return checker::check_all(output_lines, queued_submission_value.problem_id);
}

std::expected<void, error_code> judge_worker::run(){
    while(true){
        auto lease_submission_exp = lease_submission();
        if(!lease_submission_exp){
            return std::unexpected(lease_submission_exp.error());
        }

        if(lease_submission_exp->has_value()){
            const submission_dto::queued_submission& queued_submission_value =
                lease_submission_exp->value();
            const auto save_source_code_exp = save_source_code(queued_submission_value);
            if(!save_source_code_exp){
                return std::unexpected(save_source_code_exp.error());
            }

            try{
                pqxx::work transaction(db_connection_.connection());
                const auto update_submission_status_exp = submission_util::update_submission_status(
                    transaction,
                    queued_submission_value.submission_id,
                    submission_status::judging
                );
                if(!update_submission_status_exp){
                    return std::unexpected(update_submission_status_exp.error());
                }

                transaction.commit();
            }
            catch(const std::exception& exception){
                return std::unexpected(error_code::map_psql_error_code(exception));
            }

            const auto source_file_path_exp = judge_util::instance().make_source_file_path(
                queued_submission_value.submission_id,
                queued_submission_value.language
            );

            if(!source_file_path_exp){
                return std::unexpected(source_file_path_exp.error());
            }

            const auto sync_tc_exp = tc_downloader_.sync_tc(
                queued_submission_value.problem_id
            );
            
            if(!sync_tc_exp){
                return std::unexpected(sync_tc_exp.error());
            }

            const std::filesystem::path source_file_path = *source_file_path_exp;
            auto run_all_tcs_exp = tc_runner::run_all_tcs(
                source_file_path,
                queued_submission_value.problem_id
            );

            if(!run_all_tcs_exp){
                return std::unexpected(run_all_tcs_exp.error());
            }

            const auto judge_result_exp = judge_submission(
                queued_submission_value, *run_all_tcs_exp
            );

            if(!judge_result_exp){
                return std::unexpected(judge_result_exp.error());
            }

            const submission_status submission_status_value = to_submission_status(
                *judge_result_exp
            );

            const finalize_submission_data finalize_submission_data_value = make_finalize_submission_data(
                submission_status_value,
                *run_all_tcs_exp
            );

            try{
                pqxx::work transaction(db_connection_.connection());
                const auto finalize_submission_exp = submission_util::finalize_submission(
                    transaction,
                    queued_submission_value.submission_id,
                    submission_status_value,
                    finalize_submission_data_value.score,
                    finalize_submission_data_value.compile_output,
                    finalize_submission_data_value.judge_output,
                    std::nullopt
                );
                if(!finalize_submission_exp){
                    return std::unexpected(finalize_submission_exp.error());
                }

                transaction.commit();
            }
            catch(const std::exception& exception){
                return std::unexpected(error_code::map_psql_error_code(exception));
            }

            continue;
        }

        auto wait_submission_notification_exp = submission_service_.wait_submission_notification(
            notification_wait_timeout_
        );
        
        if(!wait_submission_notification_exp){
            return std::unexpected(wait_submission_notification_exp.error());
        }
    }
}

std::expected<std::optional<submission_dto::queued_submission>, error_code> judge_worker::lease_submission(){
    try{
        pqxx::work transaction(db_connection_.connection());
        auto lease_submission_exp = submission_util::lease_submission(transaction, lease_duration_);
        if(!lease_submission_exp){
            if(is_queue_empty_error(lease_submission_exp.error())){
                return std::optional<submission_dto::queued_submission>{};
            }

            return std::unexpected(lease_submission_exp.error());
        }

        transaction.commit();
        return std::optional<submission_dto::queued_submission>{std::move(*lease_submission_exp)};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> judge_worker::save_source_code(
    const submission_dto::queued_submission& queued_submission_value
){
    const auto source_file_path_exp = judge_util::instance().make_source_file_path(
        queued_submission_value.submission_id,
        queued_submission_value.language
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }
    const auto source_file_path = *source_file_path_exp;
    
    auto create_file_exp = file_util::create_file(
        source_file_path,
        queued_submission_value.source_code
    );

    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return {};
}
