#include "judge_server/testcase_downloader.hpp"

#include "common/env_utility.hpp"

#include <filesystem>
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

