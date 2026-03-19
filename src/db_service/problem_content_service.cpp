#include "db_service/problem_content_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"

#include <utility>

std::expected<problem_dto::statement, error_code> problem_content_service::get_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::statement, error_code> {
            return problem_content_util::get_statement(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<void, error_code> problem_content_service::set_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::statement& statement
){
    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_statement_exp = problem_content_util::set_statement(
                transaction,
                problem_reference_value,
                statement
            );
            if(!set_statement_exp){
                return std::unexpected(set_statement_exp.error());
            }

            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<std::vector<problem_dto::sample>, error_code> problem_content_service::list_samples(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::sample>, error_code> {
            return problem_content_util::list_samples(
                transaction,
                problem_reference_value
            );
        }
    );
}
