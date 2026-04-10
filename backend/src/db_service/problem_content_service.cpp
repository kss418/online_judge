#include "db_service/problem_content_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_repository/problem_content_repository.hpp"
#include "db_repository/problem_core_repository.hpp"
#include "db_repository/problem_snapshot_repository.hpp"

#include <utility>

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

    problem_dto::sample_mutation_result make_sample_mutation_result(
        const problem_content_dto::sample& sample_value,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value
    ){
        return problem_dto::sample_mutation_result{
            .sample_value = sample_value,
            .problem_value = make_mutation_result(
                problem_reference_value,
                version_value
            )
        };
    }
}

std::expected<problem_content_dto::limits, service_error> problem_content_service::get_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::limits, service_error> {
            return problem_core_repository::get_limits(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::mutation_result, service_error> problem_content_service::set_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::limits& limits_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto set_limits_exp = problem_core_repository::set_limits(
                transaction,
                problem_reference_value,
                limits_value
            );
            if(!set_limits_exp){
                return std::unexpected(set_limits_exp.error());
            }

            const auto version_exp = problem_core_repository::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_snapshot_repository::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            return make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<std::optional<problem_content_dto::statement>, service_error>
problem_content_service::get_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::optional<problem_content_dto::statement>, service_error> {
            return problem_content_repository::get_statement(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::mutation_result, service_error> problem_content_service::set_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::statement& statement
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto set_statement_exp = problem_content_repository::set_statement(
                transaction,
                problem_reference_value,
                statement
            );
            if(!set_statement_exp){
                return std::unexpected(set_statement_exp.error());
            }

            const auto version_exp = problem_core_repository::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_snapshot_repository::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            return make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_dto::sample_mutation_result, service_error>
problem_content_service::create_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::sample& sample_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::sample_mutation_result, service_error> {
            const auto created_sample_exp = problem_content_repository::create_sample(
                transaction,
                problem_reference_value,
                sample_value
            );
            if(!created_sample_exp){
                return std::unexpected(created_sample_exp.error());
            }

            const auto version_exp = problem_core_repository::get_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return make_sample_mutation_result(
                *created_sample_exp,
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_content_dto::sample, service_error> problem_content_service::get_sample(
    db_connection& connection,
    const problem_content_dto::sample_ref& sample_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::sample, service_error> {
            return problem_content_repository::get_sample(
                transaction,
                sample_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_content_dto::sample>, service_error>
problem_content_service::list_samples(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_content_dto::sample>, service_error> {
            return problem_content_repository::list_samples(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<void, service_error> problem_content_service::set_sample(
    db_connection& connection,
    const problem_content_dto::sample_ref& sample_reference_value,
    const problem_content_dto::sample& sample_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            const auto set_sample_exp = problem_content_repository::set_sample(
                transaction,
                sample_reference_value,
                sample_value
            );
            if(!set_sample_exp){
                return std::unexpected(set_sample_exp.error());
            }

            problem_dto::reference problem_reference_value{
                sample_reference_value.problem_id
            };
            const auto version_exp = problem_core_repository::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_snapshot_repository::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            return {};
        }
    );
}

std::expected<problem_dto::sample_mutation_result, service_error>
problem_content_service::set_sample_and_get(
    db_connection& connection,
    const problem_content_dto::sample_ref& sample_reference_value,
    const problem_content_dto::sample& sample_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::sample_mutation_result, service_error> {
            const auto set_sample_exp = problem_content_repository::set_sample(
                transaction,
                sample_reference_value,
                sample_value
            );
            if(!set_sample_exp){
                return std::unexpected(set_sample_exp.error());
            }

            problem_dto::reference problem_reference_value{
                sample_reference_value.problem_id
            };
            const auto version_exp = problem_core_repository::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_snapshot_repository::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            const auto updated_sample_exp = problem_content_repository::get_sample(
                transaction,
                sample_reference_value
            );
            if(!updated_sample_exp){
                return std::unexpected(updated_sample_exp.error());
            }

            return make_sample_mutation_result(
                *updated_sample_exp,
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_dto::mutation_result, service_error> problem_content_service::delete_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto sample_values_exp = problem_content_repository::list_samples(
                transaction,
                problem_reference_value
            );
            if(!sample_values_exp){
                return std::unexpected(sample_values_exp.error());
            }
            if(sample_values_exp->empty()){
                return std::unexpected(service_error{
                    service_error_code::validation_error,
                    "missing sample to delete"
                });
            }

            problem_content_dto::sample_ref sample_reference_value{
                .problem_id = problem_reference_value.problem_id,
                .sample_order = sample_values_exp->back().order
            };
            const auto delete_sample_exp = problem_content_repository::delete_sample(
                transaction,
                sample_reference_value
            );
            if(!delete_sample_exp){
                return std::unexpected(delete_sample_exp.error());
            }

            const auto version_exp = problem_core_repository::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto publish_snapshot_exp =
                problem_snapshot_repository::publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!publish_snapshot_exp){
                return std::unexpected(publish_snapshot_exp.error());
            }

            return make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}
