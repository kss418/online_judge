#include "db/problem_service.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_service, error_code> problem_service::create(db_connection db_connection){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return problem_service(std::move(db_connection));
}

problem_service::problem_service(db_connection connection) :
    db_connection_(std::move(connection)){}

pqxx::connection& problem_service::connection(){
    return db_connection_.connection();
}

const pqxx::connection& problem_service::connection() const{
    return db_connection_.connection();
}

std::expected<std::int64_t, error_code> problem_service::create_problem(){
    if(!db_connection_.is_connected()){
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

std::expected<limits, error_code> problem_service::get_limits(std::int64_t problem_id){
    if(!db_connection_.is_connected()){
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

std::expected<void, error_code> problem_service::set_limits(
    std::int64_t problem_id,
    std::int32_t memory_limit_mb,
    std::int32_t time_limit_ms
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || memory_limit_mb <= 0 || time_limit_ms <= 0){
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
            memory_limit_mb,
            time_limit_ms
        );

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::increase_submission_count(std::int64_t problem_id){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problem_statistics "
            "SET "
            "submission_count = submission_count + 1, "
            "updated_at = NOW() "
            "WHERE problem_id = $1",
            problem_id
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::increase_accepted_count(std::int64_t problem_id){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problem_statistics "
            "SET "
            "accepted_count = accepted_count + 1, "
            "updated_at = NOW() "
            "WHERE problem_id = $1",
            problem_id
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::set_statement(
    std::int64_t problem_id,
    const std::string& description,
    const std::string& input_format,
    const std::string& output_format,
    const std::string& note
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        transaction.exec_params(
            "INSERT INTO problem_statements("
            "problem_id, description, input_format, output_format, note, created_at, updated_at"
            ") VALUES($1, $2, $3, $4, NULLIF($5, ''), NOW(), NOW()) "
            "ON CONFLICT(problem_id) DO UPDATE "
            "SET "
            "description = EXCLUDED.description, "
            "input_format = EXCLUDED.input_format, "
            "output_format = EXCLUDED.output_format, "
            "note = NULLIF(EXCLUDED.note, ''), "
            "updated_at = NOW()",
            problem_id,
            description,
            input_format,
            output_format,
            note
        );

        const auto version_exp = increase_version(transaction, problem_id);
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

std::expected<std::int64_t, error_code> problem_service::create_sample(
    std::int64_t problem_id,
    const std::string& sample_input,
    const std::string& sample_output
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto sample_order_exp = increase_sample_count(transaction, problem_id);
        if(!sample_order_exp){
            return std::unexpected(sample_order_exp.error());
        }

        const std::int32_t sample_order = sample_order_exp.value();
        const auto create_sample_result = transaction.exec_params(
            "INSERT INTO problem_samples(problem_id, sample_order, sample_input, sample_output) "
            "VALUES($1, $2, $3, $4) "
            "RETURNING sample_id",
            problem_id,
            sample_order,
            sample_input,
            sample_output
        );

        if(create_sample_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        const auto version_exp = increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        const std::int64_t sample_id = create_sample_result[0][0].as<std::int64_t>();
        transaction.commit();
        return sample_id;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::set_sample(
    std::int64_t problem_id,
    std::int32_t sample_order,
    const std::string& sample_input,
    const std::string& sample_output
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || sample_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        transaction.exec_params(
            "INSERT INTO problem_samples(problem_id, sample_order, sample_input, sample_output) "
            "VALUES($1, $2, $3, $4) "
            "ON CONFLICT(problem_id, sample_order) DO UPDATE "
            "SET "
            "sample_input = EXCLUDED.sample_input, "
            "sample_output = EXCLUDED.sample_output",
            problem_id,
            sample_order,
            sample_input,
            sample_output
        );

        const auto version_exp = increase_version(transaction, problem_id);
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

std::expected<void, error_code> problem_service::delete_sample(
    std::int64_t problem_id,
    std::int32_t sample_order
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || sample_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto delete_result = transaction.exec_params(
            "DELETE FROM problem_samples "
            "WHERE "
            "problem_id = $1 AND "
            "sample_order = $2 AND "
            "sample_order = ("
            "SELECT MAX(sample_order) "
            "FROM problem_samples "
            "WHERE problem_id = $1"
            ")",
            problem_id,
            sample_order
        );

        if(delete_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const auto sample_count_exp = decrease_sample_count(transaction, problem_id);
        if(!sample_count_exp){
            return std::unexpected(sample_count_exp.error());
        }

        const auto version_exp = increase_version(transaction, problem_id);
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

std::expected<void, error_code> problem_service::increase_version(
    pqxx::work& transaction,
    std::int64_t problem_id
){
    const auto update_result = transaction.exec_params(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        problem_id
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<std::int32_t, error_code> problem_service::increase_sample_count(
    pqxx::work& transaction,
    std::int64_t problem_id
){
    const auto increase_result = transaction.exec_params(
        "UPDATE problem_statements "
        "SET sample_count = sample_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING sample_count",
        problem_id
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int32_t sample_order = increase_result[0][0].as<std::int32_t>();
    return sample_order;
}

std::expected<std::int32_t, error_code> problem_service::decrease_sample_count(
    pqxx::work& transaction,
    std::int64_t problem_id
){
    const auto decrease_result = transaction.exec_params(
        "UPDATE problem_statements "
        "SET sample_count = sample_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND sample_count > 0 "
        "RETURNING sample_count",
        problem_id
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int32_t sample_count = decrease_result[0][0].as<std::int32_t>();
    return sample_count;
}

std::expected<std::int32_t, error_code> problem_service::increase_testcase_count(
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

std::expected<std::int32_t, error_code> problem_service::decrease_testcase_count(
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
