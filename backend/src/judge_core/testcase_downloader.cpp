#include "judge_core/testcase_downloader.hpp"

#include "common/file_util.hpp"
#include "common/temp_dir.hpp"
#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/testcase_service.hpp"
#include "judge_core/testcase_util.hpp"

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace{
    constexpr int FILE_OPERATION_ATTEMPT_COUNT = 5;

    std::expected<void, error_code> rename_directory(
        const std::filesystem::path& source_path,
        const std::filesystem::path& destination_path
    ){
        return file_util::retry_file_operation(
            FILE_OPERATION_ATTEMPT_COUNT,
            [&]() -> std::expected<void, error_code> {
                std::error_code rename_ec;
                std::filesystem::rename(source_path, destination_path, rename_ec);
                if(rename_ec){
                    return std::unexpected(
                        error_code::create(error_code::map_errno(rename_ec.value()))
                    );
                }

                return std::expected<void, error_code>{};
            }
        );
    }
}

testcase_downloader::testcase_downloader(
    db_connection connection
) :
    connection_(std::move(connection)){}

std::expected<testcase_downloader, error_code> testcase_downloader::create(db_connection connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return testcase_downloader(std::move(connection));
}

std::expected<std::int32_t, error_code> testcase_downloader::fetch_problem_version(
    std::int64_t problem_id
){
    const problem_dto::reference problem_reference_value{problem_id};
    const auto version_exp = problem_core_service::get_version(
        connection_,
        problem_reference_value
    );
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return version_exp->version;
}

std::expected<void, error_code> testcase_downloader::sync_limit_file(
    std::int64_t problem_id,
    const std::filesystem::path& testcase_directory_path
){
    const problem_dto::reference problem_reference_value{problem_id};
    const auto limits_exp = problem_content_service::get_limits(
        connection_,
        problem_reference_value
    );
    if(!limits_exp){
        return std::unexpected(limits_exp.error());
    }

    const auto memory_limit_file_path_exp = testcase_util::instance().make_testcase_memory_limit_file_path(
        testcase_directory_path
    );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_file_path_exp = testcase_util::instance().make_testcase_time_limit_file_path(
        testcase_directory_path
    );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        memory_limit_file_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_memory_limit_file_exp = file_util::create_file(
        *memory_limit_file_path_exp,
        std::to_string(limits_exp->memory_mb)
    );
    if(!create_memory_limit_file_exp){
        return std::unexpected(create_memory_limit_file_exp.error());
    }

    const auto create_time_limit_file_exp = file_util::create_file(
        *time_limit_file_path_exp,
        std::to_string(limits_exp->time_ms)
    );
    if(!create_time_limit_file_exp){
        return std::unexpected(create_time_limit_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::sync_version_directory(
    std::int64_t problem_id,
    std::int32_t version
){
    const auto version_directory_path_exp = testcase_util::instance().make_testcase_version_directory_path(
        problem_id,
        version
    );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto version_directory_exists_exp = file_util::exists(*version_directory_path_exp);
    if(!version_directory_exists_exp){
        return std::unexpected(version_directory_exists_exp.error());
    }
    if(version_directory_exists_exp.value()){
        return {};
    }

    const auto problem_directory_path_exp = testcase_util::instance().make_testcase_problem_directory_path(
        problem_id
    );
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    const auto create_problem_directory_exp = file_util::create_directories(
        *problem_directory_path_exp
    );
    if(!create_problem_directory_exp){
        return std::unexpected(create_problem_directory_exp.error());
    }

    std::string temp_directory_pattern =
        (*problem_directory_path_exp / ("version-" + std::to_string(version) + ".tmp.XXXXXX"))
            .string();
    auto temp_directory_exp = temp_dir::create(temp_directory_pattern);
    if(!temp_directory_exp){
        return std::unexpected(temp_directory_exp.error());
    }

    const auto download_all_exp = download_all(problem_id, temp_directory_exp->get_path());
    if(!download_all_exp){
        return std::unexpected(download_all_exp.error());
    }

    const auto sync_limit_file_exp = sync_limit_file(problem_id, temp_directory_exp->get_path());
    if(!sync_limit_file_exp){
        return std::unexpected(sync_limit_file_exp.error());
    }

    const auto rename_directory_exp = rename_directory(
        temp_directory_exp->get_path(),
        *version_directory_path_exp
    );
    if(!rename_directory_exp){
        const auto exists_after_rename_exp = file_util::exists(*version_directory_path_exp);
        if(!exists_after_rename_exp){
            return std::unexpected(exists_after_rename_exp.error());
        }

        if(!exists_after_rename_exp.value()){
            return std::unexpected(rename_directory_exp.error());
        }
    }

    return {};
}

std::expected<std::filesystem::path, error_code> testcase_downloader::sync_testcases(
    std::int64_t problem_id
){
    const auto version_exp = fetch_problem_version(problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto version_directory_path_exp = testcase_util::instance().make_testcase_version_directory_path(
        problem_id,
        *version_exp
    );
    if(!version_directory_path_exp){
        return std::unexpected(version_directory_path_exp.error());
    }

    const auto version_directory_exists_exp = file_util::exists(*version_directory_path_exp);
    if(!version_directory_exists_exp){
        return std::unexpected(version_directory_exists_exp.error());
    }

    if(!version_directory_exists_exp.value()){
        const auto sync_version_directory_exp = sync_version_directory(problem_id, *version_exp);
        if(!sync_version_directory_exp){
            return std::unexpected(sync_version_directory_exp.error());
        }
    }

    return *version_directory_path_exp;
}

std::expected<void, error_code> testcase_downloader::download_all(
    std::int64_t problem_id,
    const std::filesystem::path& testcase_directory_path
){
    const problem_dto::reference problem_reference_value{problem_id};
    const auto testcase_count_exp = testcase_service::get_testcase_count(
        connection_,
        problem_reference_value
    );
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= testcase_count_exp->testcase_count; ++order){
        const auto download_one_exp = download_one(problem_id, order, testcase_directory_path);
        if(!download_one_exp){
            return std::unexpected(download_one_exp.error());
        }
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::download_one(
    std::int64_t problem_id,
    std::int32_t order,
    const std::filesystem::path& testcase_directory_path
){
    problem_dto::testcase_ref testcase_reference_value;
    testcase_reference_value.problem_id = problem_id;
    testcase_reference_value.testcase_order = order;
    const auto testcase_exp = testcase_service::get_testcase(
        connection_,
        testcase_reference_value
    );
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    const auto input_path_exp = testcase_util::instance().make_testcase_input_path(
        testcase_directory_path,
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    const auto output_path_exp = testcase_util::instance().make_testcase_output_path(
        testcase_directory_path,
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    const std::filesystem::path input_path = *input_path_exp;
    const std::filesystem::path output_path = *output_path_exp;
    const auto create_directories_exp = file_util::create_directories(
        input_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_input_exp = file_util::create_file(
        input_path,
        testcase_exp->input
    );
    
    if(!create_input_exp){
        return std::unexpected(create_input_exp.error());
    }

    const auto create_output_exp = file_util::create_file(
        output_path,
        testcase_exp->output
    );
    
    if(!create_output_exp){
        return std::unexpected(create_output_exp.error());
    }

    return {};
}
