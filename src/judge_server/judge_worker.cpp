#include "judge_server/judge_worker.hpp"
#include "common/file_utility.hpp"
#include "common/env_utility.hpp"

#include <system_error>
#include <utility>

std::expected<judge_worker, error_code> judge_worker::create(submission_service submission_service){
    auto source_root_exp = env_utility::require_env("JUDGE_SOURCE_ROOT");
    if(!source_root_exp){
        return std::unexpected(source_root_exp.error());
    }

    std::filesystem::path source_root_path = *source_root_exp;
    if(source_root_path.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::error_code create_directory_ec;
    std::filesystem::create_directories(source_root_path, create_directory_ec);
    if(create_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(create_directory_ec.value())));
    }

    return judge_worker(std::move(submission_service), std::move(source_root_path));
}

judge_worker::judge_worker(
    submission_service submission_service, std::filesystem::path source_root_path
) :
    submission_service_(std::move(submission_service)),
    source_root_path_(std::move(source_root_path)){}

bool judge_worker::is_queue_empty_error(const error_code& code){
    return code.type_ == error_type::errno_type &&
        code.code_ == static_cast<int>(errno_error::resource_temporarily_unavailable);
}

std::expected <bool, error_code> judge_worker::save_source_code(){
    auto pop_submission_exp = submission_service_.pop_submission();
    if(!pop_submission_exp){
        if(is_queue_empty_error(pop_submission_exp.error())){
            return false;
        }

        return std::unexpected(pop_submission_exp.error());
    }

    const auto source_file_path = file_utility::make_source_file_path(
        source_root_path_,
        pop_submission_exp->submission_id,
        pop_submission_exp->language
    );
    
    auto create_file_exp = file_utility::create_file(
        source_file_path,
        pop_submission_exp->source_code
    );

    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return true;
}
