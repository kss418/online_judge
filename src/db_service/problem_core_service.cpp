#include "db_service/problem_core_service.hpp"
#include "db_service/db_service_util.hpp"
#include "dto/problem_content_dto.hpp"
#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"
#include "db_util/problem_statistics_util.hpp"

std::expected<problem_dto::existence, error_code> problem_core_service::exists_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_write_transaction(
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

std::expected<problem_dto::title, error_code> problem_core_service::get_title(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::title, error_code> {
            return problem_core_util::get_title(
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
    return db_service_util::with_retry_read_transaction(
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
    db_connection& connection,
    const problem_dto::create_request& create_request_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::created, error_code> {
            const auto created_exp = problem_core_util::create_problem(
                transaction,
                create_request_value
            );
            if(!created_exp){
                return std::unexpected(created_exp.error());
            }
            const problem_dto::reference problem_reference_value{created_exp->problem_id};

            problem_content_dto::limits initial_limits_value;
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

std::expected<void, error_code> problem_core_service::update_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::update_request& update_request_value
){
    if(problem_reference_value.problem_id <= 0 || update_request_value.title.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            problem_dto::title title_value;
            title_value.value = update_request_value.title;

            const auto set_title_exp = problem_core_util::set_title(
                transaction,
                problem_reference_value,
                title_value
            );
            if(!set_title_exp){
                return std::unexpected(set_title_exp.error());
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

std::expected<void, error_code> problem_core_service::delete_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            return problem_core_util::delete_problem(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, error_code> problem_core_service::list_problems(
    db_connection& connection,
    const problem_dto::list_filter& filter_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::summary>, error_code> {
            return problem_core_util::list_problems(transaction, filter_value);
        }
    );
}
