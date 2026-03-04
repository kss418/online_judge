#include "db/problem_core_service.hpp"
#include "db/problem_service_utility.hpp"

#include <pqxx/pqxx>

#include <utility>

problem_core_service::problem_core_service(db_connection connection) :
    db_service_base<problem_core_service>(std::move(connection)){}

std::expected<std::int64_t, error_code> problem_core_service::create_problem(){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto create_problem_result = transaction.exec_params(
            "INSERT INTO problems(version) "
            "VALUES($1) "
            "RETURNING problem_id",
            1
        );

        if(create_problem_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const std::int64_t problem_id = create_problem_result[0][0].as<std::int64_t>();
        transaction.commit();
        return problem_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<limits, error_code> problem_core_service::get_limits(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto limits_query_result = transaction.exec_params(
            "SELECT memory_limit_mb, time_limit_ms "
            "FROM problem_limits "
            "WHERE problem_id = $1",
            problem_id
        );

        if(limits_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        limits limits_value;
        limits_value.memory_limit_mb = limits_query_result[0][0].as<std::int32_t>();
        limits_value.time_limit_ms = limits_query_result[0][1].as<std::int32_t>();
        return limits_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::set_limits(
    std::int64_t problem_id,
    const limits& limits_value
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || limits_value.memory_limit_mb <= 0 || limits_value.time_limit_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        transaction.exec_params(
            "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
            "VALUES($1, $2, $3, NOW()) "
            "ON CONFLICT(problem_id) DO UPDATE "
            "SET "
            "memory_limit_mb = EXCLUDED.memory_limit_mb, "
            "time_limit_ms = EXCLUDED.time_limit_ms, "
            "updated_at = NOW()",
            problem_id,
            limits_value.memory_limit_mb,
            limits_value.time_limit_ms
        );

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int64_t, error_code> problem_core_service::create_testcase(
    std::int64_t problem_id,
    const testcase& testcase_value
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto testcase_order_exp = increase_testcase_count(transaction, problem_id);
        if(!testcase_order_exp){
            return std::unexpected(testcase_order_exp.error());
        }

        const std::int32_t testcase_order = testcase_order_exp.value();
        const auto create_testcase_result = transaction.exec_params(
            "INSERT INTO problem_testcases(problem_id, testcase_order, testcase_input, testcase_output) "
            "VALUES($1, $2, $3, $4) "
            "RETURNING testcase_id",
            problem_id,
            testcase_order,
            testcase_value.testcase_input,
            testcase_value.testcase_output
        );

        if(create_testcase_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        const std::int64_t testcase_id = create_testcase_result[0][0].as<std::int64_t>();
        transaction.commit();
        return testcase_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<testcase, error_code> problem_core_service::get_testcase(
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto testcase_query_result = transaction.exec_params(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 AND testcase_order = $2",
            problem_id,
            testcase_order
        );

        if(testcase_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        testcase testcase_value;
        testcase_value.testcase_id = testcase_query_result[0][0].as<std::int64_t>();
        testcase_value.testcase_order = testcase_query_result[0][1].as<std::int32_t>();
        testcase_value.testcase_input = testcase_query_result[0][2].as<std::string>();
        testcase_value.testcase_output = testcase_query_result[0][3].as<std::string>();
        return testcase_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<testcase>, error_code> problem_core_service::list_testcases(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto testcases_query_result = transaction.exec_params(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 "
            "ORDER BY testcase_order ASC",
            problem_id
        );

        std::vector<testcase> testcase_values;
        testcase_values.reserve(testcases_query_result.size());
        for(const auto& row : testcases_query_result){
            testcase testcase_value;
            testcase_value.testcase_id = row[0].as<std::int64_t>();
            testcase_value.testcase_order = row[1].as<std::int32_t>();
            testcase_value.testcase_input = row[2].as<std::string>();
            testcase_value.testcase_output = row[3].as<std::string>();
            testcase_values.push_back(std::move(testcase_value));
        }

        return testcase_values;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::set_testcase(
    std::int64_t problem_id,
    const testcase& testcase_value
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || testcase_value.testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problem_testcases "
            "SET "
            "testcase_input = $3, "
            "testcase_output = $4 "
            "WHERE problem_id = $1 AND testcase_order = $2",
            problem_id,
            testcase_value.testcase_order,
            testcase_value.testcase_input,
            testcase_value.testcase_output
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::delete_testcase(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto delete_result = transaction.exec_params(
            "DELETE FROM problem_testcases "
            "WHERE "
            "problem_id = $1 AND "
            "testcase_order = ("
            "SELECT MAX(testcase_order) "
            "FROM problem_testcases "
            "WHERE problem_id = $1"
            ")",
            problem_id
        );

        if(delete_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const auto testcase_count_exp = decrease_testcase_count(transaction, problem_id);
        if(!testcase_count_exp){
            return std::unexpected(testcase_count_exp.error());
        }

        const auto version_exp = problem_service_utility::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int32_t, error_code> problem_core_service::increase_testcase_count(
    pqxx::work& transaction,
    std::int64_t problem_id
){
    const auto increase_result = transaction.exec_params(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        problem_id
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int32_t testcase_count = increase_result[0][0].as<std::int32_t>();
    return testcase_count;
}

std::expected<std::int32_t, error_code> problem_core_service::decrease_testcase_count(
    pqxx::work& transaction,
    std::int64_t problem_id
){
    const auto decrease_result = transaction.exec_params(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND testcase_count > 0 "
        "RETURNING testcase_count",
        problem_id
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int32_t testcase_count = decrease_result[0][0].as<std::int32_t>();
    return testcase_count;
}
