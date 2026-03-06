#include "judge_server/testcase_loader.hpp"

#include "common/env_utility.hpp"
#include "common/file_utility.hpp"

#include <pqxx/pqxx>

#include <string>
#include <system_error>
#include <utility>

std::expected<testcase_loader, error_code> testcase_loader::create(db_connection db_connection_value){
    if(!db_connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    auto testcase_root_path_exp = env_utility::require_env("JUDGE_TESTCASE_PATH");
    if(!testcase_root_path_exp){
        return std::unexpected(testcase_root_path_exp.error());
    }

    std::filesystem::path testcase_root_path = *testcase_root_path_exp;
    if(testcase_root_path.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::error_code create_directory_ec;
    std::filesystem::create_directories(testcase_root_path, create_directory_ec);
    if(create_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(create_directory_ec.value())));
    }

    return testcase_loader(
        std::move(db_connection_value),
        std::move(testcase_root_path)
    );
}

std::expected<testcase_load_result, error_code> testcase_loader::load(std::int64_t problem_id){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::vector<testcase> testcase_values;
    try{
        pqxx::work transaction(db_connection_.connection());
        const auto testcases_query_result = transaction.exec(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 "
            "ORDER BY testcase_order ASC",
            pqxx::params{problem_id}
        );

        testcase_values.reserve(testcases_query_result.size());
        for(const auto& row : testcases_query_result){
            testcase testcase_value;
            testcase_value.testcase_id = row[0].as<std::int64_t>();
            testcase_value.testcase_order = row[1].as<std::int32_t>();
            testcase_value.testcase_input = row[2].as<std::string>();
            testcase_value.testcase_output = row[3].as<std::string>();
            testcase_values.push_back(std::move(testcase_value));
        }
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }

    const auto problem_directory_path = make_problem_directory_path(testcase_root_path_, problem_id);
    auto reset_problem_directory_exp = reset_problem_directory(problem_directory_path);
    if(!reset_problem_directory_exp){
        return std::unexpected(reset_problem_directory_exp.error());
    }

    testcase_load_result load_result_value;
    load_result_value.problem_id = problem_id;
    load_result_value.problem_directory_path = problem_directory_path;
    load_result_value.loaded_testcases.reserve(testcase_values.size());

    for(const auto& testcase_value : testcase_values){
        auto loaded_testcase_exp = save_testcase(problem_directory_path, testcase_value);
        if(!loaded_testcase_exp){
            return std::unexpected(loaded_testcase_exp.error());
        }

        load_result_value.loaded_testcases.push_back(std::move(*loaded_testcase_exp));
    }

    return load_result_value;
}

testcase_loader::testcase_loader(
    db_connection db_connection_value,
    std::filesystem::path testcase_root_path
) :
    db_connection_(std::move(db_connection_value)),
    testcase_root_path_(std::move(testcase_root_path)){}

std::expected<void, error_code> testcase_loader::reset_problem_directory(
    const std::filesystem::path& problem_directory_path
) const{
    std::error_code remove_directory_ec;
    std::filesystem::remove_all(problem_directory_path, remove_directory_ec);
    if(remove_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(remove_directory_ec.value())));
    }

    std::error_code create_directory_ec;
    std::filesystem::create_directories(problem_directory_path, create_directory_ec);
    if(create_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(create_directory_ec.value())));
    }

    return {};
}

std::expected<loaded_testcase, error_code> testcase_loader::save_testcase(
    const std::filesystem::path& problem_directory_path,
    const testcase& testcase_value
) const{
    if(testcase_value.testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto input_file_path = make_testcase_input_path(
        problem_directory_path,
        testcase_value.testcase_order
    );
    auto create_input_file_exp = file_utility::create_file(
        input_file_path,
        testcase_value.testcase_input
    );
    if(!create_input_file_exp){
        return std::unexpected(create_input_file_exp.error());
    }

    const auto output_file_path = make_testcase_output_path(
        problem_directory_path,
        testcase_value.testcase_order
    );
    auto create_output_file_exp = file_utility::create_file(
        output_file_path,
        testcase_value.testcase_output
    );
    if(!create_output_file_exp){
        return std::unexpected(create_output_file_exp.error());
    }

    loaded_testcase loaded_testcase_value;
    loaded_testcase_value.testcase_order = testcase_value.testcase_order;
    loaded_testcase_value.input_file_path = input_file_path;
    loaded_testcase_value.output_file_path = output_file_path;
    return loaded_testcase_value;
}

std::filesystem::path testcase_loader::make_problem_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id
){
    return testcase_root_path / std::to_string(problem_id);
}

std::filesystem::path testcase_loader::make_testcase_input_path(
    const std::filesystem::path& problem_directory_path,
    std::int32_t testcase_order
){
    return problem_directory_path / (std::to_string(testcase_order) + ".in");
}

std::filesystem::path testcase_loader::make_testcase_output_path(
    const std::filesystem::path& problem_directory_path,
    std::int32_t testcase_order
){
    return problem_directory_path / (std::to_string(testcase_order) + ".out");
}
