#include "judge_server/judge_worker.hpp"
#include "common/env_utility.hpp"

#include <fstream>
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

std::expected<bool, error_code> judge_worker::save_source_code(){
    auto pop_submission_exp = submission_service_.pop_submission();
    if(!pop_submission_exp){
        if(is_queue_empty_error(pop_submission_exp.error())){
            return false;
        }

        return std::unexpected(pop_submission_exp.error());
    }

    std::error_code create_directory_ec;
    std::filesystem::create_directories(source_root_path_, create_directory_ec);
    if(create_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(create_directory_ec.value())));
    }

    const auto source_file_path = make_source_file_path(
        pop_submission_exp->submission_id,
        pop_submission_exp->language
    );
    
    auto write_source_code_exp = write_source_code(source_file_path, pop_submission_exp->source_code);
    if(!write_source_code_exp){
        return std::unexpected(write_source_code_exp.error());
    }

    return true;
}

std::expected <void, error_code> judge_worker::write_source_code(
    const std::filesystem::path& source_path, std::string_view source_code
) const{
    std::ofstream source_file(source_path, std::ios::out | std::ios::trunc | std::ios::binary);
    if(!source_file.is_open()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    source_file.write(source_code.data(), static_cast<std::streamsize>(source_code.size()));
    if(!source_file.good()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    return {};
}

std::filesystem::path judge_worker::make_source_file_path(
    std::int64_t submission_id, std::string_view language
) const{
    std::string_view extension = ".txt";
    if(language == "cpp"){
        extension = ".cpp";
    }
    else if(language == "python"){
        extension = ".py";
    }
    else if(language == "java"){
        extension = ".java";
    }

    return source_root_path_ / (std::to_string(submission_id) + std::string(extension));
}
