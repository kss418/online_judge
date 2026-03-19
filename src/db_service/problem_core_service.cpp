#include "db_service/problem_core_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"
#include "db_util/problem_statistics_util.hpp"

std::expected<problem_dto::existence, error_code> problem_core_service::exists_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::existence, error_code> {
            return problem_core_util::exists_problem(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::version, error_code> problem_core_service::get_version(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::version, error_code> {
            return problem_core_util::get_version(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::created, error_code> problem_core_service::create_problem(
    db_connection& connection
){
    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::created, error_code> {
            const auto created_exp = problem_core_util::create_problem(
                transaction
            );
            if(!created_exp){
                return std::unexpected(created_exp.error());
            }
            const problem_dto::reference problem_reference_value{created_exp->problem_id};

            problem_dto::limits initial_limits_value;
            initial_limits_value.memory_mb = problem_core_service::INITIAL_MEMORY_LIMIT_MB;
            initial_limits_value.time_ms = problem_core_service::INITIAL_TIME_LIMIT_MS;

            const auto set_limits_exp = problem_core_util::set_limits(
                transaction,
                problem_reference_value,
                initial_limits_value
            );
            if(!set_limits_exp){
                return std::unexpected(set_limits_exp.error());
            }

            const auto create_problem_statistics_exp =
                problem_statistics_util::create_problem_statistics(
                    transaction,
                    problem_reference_value
                );
            if(!create_problem_statistics_exp){
                return std::unexpected(create_problem_statistics_exp.error());
            }

            const auto ensure_statement_exp = problem_content_util::ensure_statement_row(
                transaction,
                problem_reference_value
            );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            return *created_exp;
        }
    );
}

std::expected<problem_dto::limits, error_code> problem_core_service::get_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::limits, error_code> {
            return problem_core_util::get_limits(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<void, error_code> problem_core_service::set_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::limits& limits_value
){
    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_limits_exp = problem_core_util::set_limits(
                transaction,
                problem_reference_value,
                limits_value
            );
            if(!set_limits_exp){
                return std::unexpected(set_limits_exp.error());
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
