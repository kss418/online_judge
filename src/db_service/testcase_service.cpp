#include "db_service/testcase_service.hpp"

#include "db_util/problem_service_util.hpp"
#include "db_util/testcase_service_util.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_dto::tc, error_code> tc_service::create_tc(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::tc& tc_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto tc_order_exp = tc_service_util::increase_tc_count(
            transaction,
            problem_id
        );
        if(!tc_order_exp){
            return std::unexpected(tc_order_exp.error());
        }

        const std::int32_t tc_order = tc_order_exp.value();
        const auto create_tc_result = transaction.exec(
            "INSERT INTO problem_testcases(problem_id, testcase_order, testcase_input, testcase_output) "
            "VALUES($1, $2, $3, $4) "
            "RETURNING testcase_id",
            pqxx::params{
                problem_id,
                tc_order,
                tc_value.input,
                tc_value.output
            }
        );

        if(create_tc_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const auto version_exp = problem_service_util::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        problem_dto::tc created_tc_value = tc_value;
        created_tc_value.id = create_tc_result[0][0].as<std::int64_t>();
        created_tc_value.order = tc_order;
        transaction.commit();
        return created_tc_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<problem_dto::tc, error_code> tc_service::get_tc(
    db_connection& connection,
    std::int64_t problem_id,
    std::int32_t tc_order
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || tc_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto tc_query_result = transaction.exec(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 AND testcase_order = $2",
            pqxx::params{problem_id, tc_order}
        );

        if(tc_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        problem_dto::tc tc_value;
        tc_value.id = tc_query_result[0][0].as<std::int64_t>();
        tc_value.order = tc_query_result[0][1].as<std::int32_t>();
        tc_value.input = tc_query_result[0][2].as<std::string>();
        tc_value.output = tc_query_result[0][3].as<std::string>();
        return tc_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int32_t, error_code> tc_service::get_tc_count(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto tc_count_query_result = transaction.exec(
            "SELECT COUNT(*) "
            "FROM problem_testcases "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(tc_count_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        return tc_count_query_result[0][0].as<std::int32_t>();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<problem_dto::tc>, error_code> tc_service::list_tcs(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto tcs_query_result = transaction.exec(
            "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
            "FROM problem_testcases "
            "WHERE problem_id = $1 "
            "ORDER BY testcase_order ASC",
            pqxx::params{problem_id}
        );

        std::vector<problem_dto::tc> tc_values;
        tc_values.reserve(tcs_query_result.size());
        for(const auto& row : tcs_query_result){
            problem_dto::tc tc_value;
            tc_value.id = row[0].as<std::int64_t>();
            tc_value.order = row[1].as<std::int32_t>();
            tc_value.input = row[2].as<std::string>();
            tc_value.output = row[3].as<std::string>();
            tc_values.push_back(std::move(tc_value));
        }

        return tc_values;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> tc_service::set_tc(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::tc& tc_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || tc_value.order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto update_result = transaction.exec(
            "UPDATE problem_testcases "
            "SET "
            "testcase_input = $3, "
            "testcase_output = $4 "
            "WHERE problem_id = $1 AND testcase_order = $2",
            pqxx::params{
                problem_id,
                tc_value.order,
                tc_value.input,
                tc_value.output
            }
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const auto version_exp = problem_service_util::increase_version(transaction, problem_id);
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

std::expected<void, error_code> tc_service::delete_tc(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto delete_result = transaction.exec(
            "DELETE FROM problem_testcases "
            "WHERE "
            "problem_id = $1 AND "
            "testcase_order = ("
            "SELECT MAX(testcase_order) "
            "FROM problem_testcases "
            "WHERE problem_id = $1"
            ")",
            pqxx::params{problem_id}
        );

        if(delete_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const auto tc_count_exp = tc_service_util::decrease_tc_count(
            transaction,
            problem_id
        );
        if(!tc_count_exp){
            return std::unexpected(tc_count_exp.error());
        }

        const auto version_exp = problem_service_util::increase_version(transaction, problem_id);
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
