#include "db/problem_content_service.hpp"

#include <pqxx/pqxx>

#include <utility>

problem_content_service::problem_content_service(db_connection connection) :
    db_service_base<problem_content_service>(std::move(connection)){}

std::expected<void, error_code> problem_content_service::set_statement(
    std::int64_t problem_id,
    const std::string& description,
    const std::string& input_format,
    const std::string& output_format,
    const std::string& note
){
    if(!is_connected()){
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

std::expected<std::int64_t, error_code> problem_content_service::create_sample(
    std::int64_t problem_id,
    const std::string& sample_input,
    const std::string& sample_output
){
    if(!is_connected()){
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

std::expected<void, error_code> problem_content_service::set_sample(
    std::int64_t problem_id,
    std::int32_t sample_order,
    const std::string& sample_input,
    const std::string& sample_output
){
    if(!is_connected()){
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

std::expected<void, error_code> problem_content_service::delete_sample(
    std::int64_t problem_id,
    std::int32_t sample_order
){
    if(!is_connected()){
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

std::expected<void, error_code> problem_content_service::increase_version(
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

std::expected<std::int32_t, error_code> problem_content_service::increase_sample_count(
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

    const std::int32_t sample_count = increase_result[0][0].as<std::int32_t>();
    return sample_count;
}

std::expected<std::int32_t, error_code> problem_content_service::decrease_sample_count(
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

std::expected<std::int32_t, error_code> problem_content_service::increase_testcase_count(
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

std::expected<std::int32_t, error_code> problem_content_service::decrease_testcase_count(
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
