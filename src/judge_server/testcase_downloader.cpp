#include "judge_server/testcase_downloader.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"
#include "db/problem_core_service.hpp"
#include "db/testcase_service.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

testcase_downloader::testcase_downloader(
    db_connection connection, std::filesystem::path root_path
) :
    connection_(std::move(connection)),
    root_path_(std::move(root_path)){}

std::expected<testcase_downloader, error_code> testcase_downloader::create(db_connection connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    const auto root_path_exp = env_utility::require_env("TESTCASE_PATH");
    if(!root_path_exp){
        return std::unexpected(root_path_exp.error());
    }

    std::filesystem::path root_path = *root_path_exp;
    if(root_path.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return testcase_downloader(std::move(connection), std::move(root_path));
}

std::filesystem::path testcase_downloader::make_problem_directory_path(std::int64_t problem_id) const{
    return root_path_ / std::to_string(problem_id);
}

std::filesystem::path testcase_downloader::make_input_path(
    std::int64_t problem_id, std::int32_t order
) const{
    return make_problem_directory_path(problem_id) / (std::to_string(order) + ".in");
}

std::filesystem::path testcase_downloader::make_output_path(
    std::int64_t problem_id, std::int32_t order
) const{
    return make_problem_directory_path(problem_id) / (std::to_string(order) + ".out");
}

std::filesystem::path testcase_downloader::make_version_file_path(std::int64_t problem_id) const{
    return make_problem_directory_path(problem_id) / "version";
}

std::expected<std::int32_t, error_code> testcase_downloader::read_version_file(std::int64_t problem_id) const{
    const std::filesystem::path version_file_path = make_version_file_path(problem_id);
    const auto version_file_exists_exp = file_utility::exists(version_file_path);
    if(!version_file_exists_exp){
        return std::unexpected(version_file_exists_exp.error());
    }

    if(!version_file_exists_exp.value()){
        return std::unexpected(error_code::create(errno_error::file_not_found));
    }

    std::ifstream version_file(version_file_path);
    if(!version_file.is_open()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    std::int32_t local_version = 0;
    version_file >> local_version;
    if(version_file.bad()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }
    
    if(!version_file){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    version_file >> std::ws;
    if(!version_file.eof()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return local_version;
}

std::expected<bool, error_code> testcase_downloader::is_latest(std::int64_t problem_id){
    const auto version_exp = problem_core_service::get_version(connection_, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto local_version_exp = read_version_file(problem_id);
    if(!local_version_exp){
        const error_code local_version_error = local_version_exp.error();
        if(
            local_version_error.type_ == error_type::errno_type &&
            (
                local_version_error.code_ == static_cast<int>(errno_error::file_not_found) ||
                local_version_error.code_ == static_cast<int>(errno_error::invalid_argument)
            )
        ){
            return false;
        }

        return std::unexpected(local_version_error);
    }

    return local_version_exp.value() == version_exp.value();
}

std::expected<void, error_code> testcase_downloader::sync_version_file(std::int64_t problem_id){
    const auto version_exp = problem_core_service::get_version(connection_, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto local_version_exp = read_version_file(problem_id);
    if(local_version_exp){
        if(local_version_exp.value() == version_exp.value()){
            return {};
        }
    }
    else{
        const error_code local_version_error = local_version_exp.error();
        if(
            local_version_error.type_ != error_type::errno_type ||
            (
                local_version_error.code_ != static_cast<int>(errno_error::file_not_found) &&
                local_version_error.code_ != static_cast<int>(errno_error::invalid_argument)
            )
        ){
            return std::unexpected(local_version_error);
        }
    }

    const std::filesystem::path version_file_path = make_version_file_path(problem_id);
    const auto create_directories_exp = file_utility::create_directories(
        version_file_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_version_file_exp = file_utility::create_file(
        version_file_path,
        std::to_string(version_exp.value())
    );
    
    if(!create_version_file_exp){
        return std::unexpected(create_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::download_all(std::int64_t problem_id){
    const auto testcase_count_exp = testcase_service::get_testcase_count(connection_, problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= testcase_count_exp.value(); ++order){
        const auto download_one_exp = download_one(problem_id, order);
        if(!download_one_exp){
            return std::unexpected(download_one_exp.error());
        }
    }

    const auto sync_version_file_exp = sync_version_file(problem_id);
    if(!sync_version_file_exp){
        return std::unexpected(sync_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::download_one(
    std::int64_t problem_id, std::int32_t order
){
    const auto testcase_exp = testcase_service::get_testcase(connection_, problem_id, order);
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    const std::filesystem::path input_path = make_input_path(problem_id, order);
    const std::filesystem::path output_path = make_output_path(problem_id, order);
    const auto create_directories_exp = file_utility::create_directories(input_path.parent_path());
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_input_exp = file_utility::create_file(
        input_path,
        testcase_exp->testcase_input
    );
    
    if(!create_input_exp){
        return std::unexpected(create_input_exp.error());
    }

    const auto create_output_exp = file_utility::create_file(
        output_path,
        testcase_exp->testcase_output
    );
    
    if(!create_output_exp){
        return std::unexpected(create_output_exp.error());
    }

    return {};
}
