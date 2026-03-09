#include "judge_server/judge_worker.hpp"

#include "common/file_utility.hpp"

#include <utility>

std::expected<judge_worker, error_code> judge_worker::create(submission_service submission_service){
    const auto source_directory_path_exp = file_utility::instance().make_source_directory_path();
    if(!source_directory_path_exp){
        return std::unexpected(source_directory_path_exp.error());
    }

    const auto create_directories_exp = file_utility::instance().create_directories(
        *source_directory_path_exp
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    auto listen_submission_queue_exp = submission_service.listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    auto code_runner_exp = code_runner::create();
    if(!code_runner_exp){
        return std::unexpected(code_runner_exp.error());
    }

    return judge_worker(std::move(submission_service), std::move(*code_runner_exp));
}

judge_worker::judge_worker(submission_service submission_service, code_runner code_runner) :
    submission_service_(std::move(submission_service)),
    code_runner_(std::move(code_runner)){}

bool judge_worker::is_queue_empty_error(const error_code& code){
    return code.type_ == error_type::errno_type &&
        code.code_ == static_cast<int>(errno_error::resource_temporarily_unavailable);
}

std::expected<void, error_code> judge_worker::run(){
    while(true){
        auto save_source_code_exp = save_source_code();
        if(!save_source_code_exp){
            return std::unexpected(save_source_code_exp.error());
        }

        if(save_source_code_exp->has_value()){
            const queued_submission& queued_submission_value = save_source_code_exp->value();
            const auto source_file_path_exp = file_utility::instance().make_source_file_path(
                queued_submission_value.submission_id,
                queued_submission_value.language
            );
            if(!source_file_path_exp){
                return std::unexpected(source_file_path_exp.error());
            }
            const std::filesystem::path source_file_path = *source_file_path_exp;
            auto run_all_testcases_exp = code_runner_.run_all_testcases(
                source_file_path,
                queued_submission_value.problem_id
            );
            if(!run_all_testcases_exp){
                return std::unexpected(run_all_testcases_exp.error());
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

std::expected<std::optional<queued_submission>, error_code> judge_worker::save_source_code(){
    auto pop_submission_exp = submission_service_.pop_submission();
    if(!pop_submission_exp){
        if(is_queue_empty_error(pop_submission_exp.error())){
            return std::optional<queued_submission>{};
        }

        return std::unexpected(pop_submission_exp.error());
    }

    const auto source_file_path_exp = file_utility::instance().make_source_file_path(
        pop_submission_exp->submission_id,
        pop_submission_exp->language
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }
    const auto source_file_path = *source_file_path_exp;
    
    auto create_file_exp = file_utility::instance().create_file(
        source_file_path,
        pop_submission_exp->source_code
    );

    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return std::move(*pop_submission_exp);
}
