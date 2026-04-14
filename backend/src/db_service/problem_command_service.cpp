#include "db_service/problem_command_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_service/problem_version_publish_service.hpp"
#include "dto/problem_content_dto.hpp"
#include "db_repository/problem_content_repository.hpp"
#include "db_repository/problem_mutation_repository.hpp"
#include "db_repository/problem_statistics_repository.hpp"

namespace{
    problem_dto::mutation_result make_mutation_result(
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value
    ){
        return problem_dto::mutation_result{
            .problem_id = problem_reference_value.problem_id,
            .version = version_value.version
        };
    }
}

std::expected<problem_dto::created, service_error> problem_command_service::create_problem(
    db_connection& connection,
    const problem_dto::create_request& create_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::created, service_error> {
            const auto created_exp = problem_mutation_repository::create_problem(
                transaction,
                create_request_value
            );
            if(!created_exp){
                return std::unexpected(created_exp.error());
            }
            const problem_dto::reference problem_reference_value{created_exp->problem_id};

            problem_content_dto::limits initial_limits_value;
            initial_limits_value.memory_mb = problem_command_service::INITIAL_MEMORY_LIMIT_MB;
            initial_limits_value.time_ms = problem_command_service::INITIAL_TIME_LIMIT_MS;

            const auto set_limits_exp = problem_mutation_repository::set_limits(
                transaction,
                problem_reference_value,
                initial_limits_value
            );
            if(!set_limits_exp){
                return std::unexpected(set_limits_exp.error());
            }

            const auto create_problem_statistics_exp =
                problem_statistics_repository::create_problem_statistics(
                    transaction,
                    problem_reference_value
                );
            if(!create_problem_statistics_exp){
                return std::unexpected(create_problem_statistics_exp.error());
            }

            const auto ensure_statement_exp =
                problem_content_repository::ensure_statement_row(
                    transaction,
                    problem_reference_value
                );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_version_publish_service::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            return *created_exp;
        }
    );
}

std::expected<problem_dto::mutation_result, service_error> problem_command_service::update_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::update_request& update_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            problem_dto::title title_value;
            title_value.value = update_request_value.title;

            const auto set_title_exp = problem_mutation_repository::set_title(
                transaction,
                problem_reference_value,
                title_value
            );
            if(!set_title_exp){
                return std::unexpected(set_title_exp.error());
            }

            const auto version_exp =
                problem_version_publish_service::increase_version_and_publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<void, service_error> problem_command_service::delete_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            return problem_mutation_repository::delete_problem(
                transaction,
                problem_reference_value
            );
        }
    );
}
