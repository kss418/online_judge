#include "db_service/problem_content_service.hpp"
#include "db_util/problem_util.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_dto::statement, error_code> problem_content_service::get_statement(
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
        const auto statement_query_result = transaction.exec(
            "SELECT description, input_format, output_format, note "
            "FROM problem_statements "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(statement_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        problem_dto::statement statement;
        statement.description = statement_query_result[0][0].as<std::string>();
        statement.input_format = statement_query_result[0][1].as<std::string>();
        statement.output_format = statement_query_result[0][2].as<std::string>();
        if(!statement_query_result[0][3].is_null()){
            statement.note = statement_query_result[0][3].as<std::string>();
        }

        if(
            statement.description.empty() &&
            statement.input_format.empty() &&
            statement.output_format.empty() &&
            !statement.note
        ){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return statement;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_content_service::set_statement(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::statement& statement
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        const std::string note_value = statement.note.value_or("");
        pqxx::work transaction(connection.connection());
        transaction.exec(
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
            pqxx::params{
                problem_id,
                statement.description,
                statement.input_format,
                statement.output_format,
                note_value
            }
        );

        const auto version_exp = problem_util::increase_version(transaction, problem_id);
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

std::expected<std::vector<problem_dto::sample>, error_code> problem_content_service::list_samples(
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
        const auto samples_query_result = transaction.exec(
            "SELECT sample_id, sample_order, sample_input, sample_output "
            "FROM problem_samples "
            "WHERE problem_id = $1 "
            "ORDER BY sample_order ASC",
            pqxx::params{problem_id}
        );

        std::vector<problem_dto::sample> sample_values;
        sample_values.reserve(samples_query_result.size());
        for(const auto& row : samples_query_result){
            problem_dto::sample sample_value;
            sample_value.id = row[0].as<std::int64_t>();
            sample_value.order = row[1].as<std::int32_t>();
            sample_value.input = row[2].as<std::string>();
            sample_value.output = row[3].as<std::string>();
            sample_values.push_back(std::move(sample_value));
        }

        return sample_values;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
