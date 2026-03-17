#include "db/submission_core_service.hpp"
#include "db/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>

std::expected<std::int64_t, error_code> submission_core_service::create_submission(
    db_connection& connection,
    std::int64_t user_id,
    std::int64_t problem_id,
    std::string_view language,
    std::string_view source_code
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(user_id <= 0 || problem_id <= 0 || language.empty() || source_code.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_submission_exp = submission_util::create_submission(
            transaction,
            user_id,
            problem_id,
            std::string(language),
            std::string(source_code)
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
