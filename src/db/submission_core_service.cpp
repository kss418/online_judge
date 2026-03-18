#include "db/submission_core_service.hpp"
#include "db/submission_util.hpp"

#include <pqxx/pqxx>

std::expected<std::int64_t, error_code> submission_core_service::create_submission(
    db_connection& connection,
    std::int64_t user_id,
    std::int64_t problem_id,
    const submission_dto::source& source_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        user_id <= 0 ||
        problem_id <= 0 ||
        source_value.language.empty() ||
        source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_submission_exp = submission_util::create_submission(
            transaction,
            user_id,
            problem_id,
            source_value
        );
        if(!create_submission_exp){
            return std::unexpected(create_submission_exp.error());
        }

        transaction.commit();
        return create_submission_exp.value();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<submission_dto::detail, error_code> submission_core_service::get_submission(
    db_connection& connection,
    std::int64_t submission_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(submission_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto submission_detail_query = transaction.exec(
            "SELECT "
            "submission_id, "
            "user_id, "
            "problem_id, "
            "language, "
            "source_code, "
            "status::text, "
            "score, "
            "compile_output, "
            "judge_output, "
            "created_at::text, "
            "updated_at::text "
            "FROM submissions "
            "WHERE submission_id = $1",
            pqxx::params{submission_id}
        );
        if(submission_detail_query.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        submission_dto::detail detail_value;
        detail_value.submission_id = submission_detail_query[0][0].as<std::int64_t>();
        detail_value.user_id = submission_detail_query[0][1].as<std::int64_t>();
        detail_value.problem_id = submission_detail_query[0][2].as<std::int64_t>();
        detail_value.language = submission_detail_query[0][3].as<std::string>();
        detail_value.source_code = submission_detail_query[0][4].as<std::string>();
        detail_value.status = submission_detail_query[0][5].as<std::string>();

        if(!submission_detail_query[0][6].is_null()){
            detail_value.score = submission_detail_query[0][6].as<std::int16_t>();
        }
        if(!submission_detail_query[0][7].is_null()){
            detail_value.compile_output = submission_detail_query[0][7].as<std::string>();
        }
        if(!submission_detail_query[0][8].is_null()){
            detail_value.judge_output = submission_detail_query[0][8].as<std::string>();
        }

        detail_value.created_at = submission_detail_query[0][9].as<std::string>();
        detail_value.updated_at = submission_detail_query[0][10].as<std::string>();
        return detail_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
