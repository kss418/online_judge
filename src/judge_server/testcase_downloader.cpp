#include "judge_server/testcase_downloader.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"
#include "db/testcase_service.hpp"

#include <filesystem>
#include <string>
#include <system_error>
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

std::filesystem::path testcase_downloader::make_input_path(
    std::int64_t problem_id, std::int32_t order
) const{
    return root_path_ / std::to_string(problem_id) / (std::to_string(order) + ".in");
}

std::filesystem::path testcase_downloader::make_output_path(
    std::int64_t problem_id, std::int32_t order
) const{
    return root_path_ / std::to_string(problem_id) / (std::to_string(order) + ".out");
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
    std::error_code create_directories_ec;
    std::filesystem::create_directories(input_path.parent_path(), create_directories_ec);
    if(create_directories_ec){
        return std::unexpected(
            error_code::create(error_code::map_errno(create_directories_ec.value()))
        );
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
