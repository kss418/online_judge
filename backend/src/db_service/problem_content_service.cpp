#include "db_service/problem_content_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_repository/problem_content_repository.hpp"
#include "db_repository/problem_core_repository.hpp"

#include <utility>

std::expected<problem_content_dto::limits, error_code> problem_content_service::get_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::limits, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                problem_core_repository::get_limits(
                    transaction,
                    problem_reference_value
                )
            );
        }
    );
}

std::expected<void, error_code> problem_content_service::set_limits(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::limits& limits_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_limits_exp = db_service_util::map_repository_error_to_http_error(
                problem_core_repository::set_limits(
                    transaction,
                    problem_reference_value,
                    limits_value
                )
            );
            if(!set_limits_exp){
                return std::unexpected(set_limits_exp.error());
            }

            const auto version_exp = db_service_util::map_repository_error_to_http_error(
                problem_core_repository::increase_version(
                    transaction,
                    problem_reference_value
                )
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<problem_content_dto::statement, error_code> problem_content_service::get_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::statement, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                problem_content_repository::get_statement(
                    transaction,
                    problem_reference_value
                )
            );
        }
    );
}

std::expected<void, error_code> problem_content_service::set_statement(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::statement& statement
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_statement_exp = db_service_util::map_repository_error_to_http_error(
                problem_content_repository::set_statement(
                    transaction,
                    problem_reference_value,
                    statement
                )
            );
            if(!set_statement_exp){
                return std::unexpected(set_statement_exp.error());
            }

            const auto version_exp = db_service_util::map_repository_error_to_http_error(
                problem_core_repository::increase_version(
                    transaction,
                    problem_reference_value
                )
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<problem_content_dto::sample, error_code> problem_content_service::create_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::sample& sample_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_content_dto::sample, error_code> {
            const auto created_sample_exp =
                db_service_util::map_repository_error_to_http_error(
                    problem_content_repository::create_sample(
                        transaction,
                        problem_reference_value,
                        sample_value
                    )
                );
            if(!created_sample_exp){
                return std::unexpected(created_sample_exp.error());
            }

            return *created_sample_exp;
        }
    );
}

std::expected<problem_content_dto::sample, error_code> problem_content_service::get_sample(
    db_connection& connection,
    const problem_content_dto::sample_ref& sample_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::sample, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                problem_content_repository::get_sample(
                    transaction,
                    sample_reference_value
                )
            );
        }
    );
}

std::expected<std::vector<problem_content_dto::sample>, error_code>
problem_content_service::list_samples(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_content_dto::sample>, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                problem_content_repository::list_samples(
                    transaction,
                    problem_reference_value
                )
            );
        }
    );
}

std::expected<void, error_code> problem_content_service::set_sample(
    db_connection& connection,
    const problem_content_dto::sample_ref& sample_reference_value,
    const problem_content_dto::sample& sample_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_sample_exp = db_service_util::map_repository_error_to_http_error(
                problem_content_repository::set_sample(
                    transaction,
                    sample_reference_value,
                    sample_value
                )
            );
            if(!set_sample_exp){
                return std::unexpected(set_sample_exp.error());
            }

            problem_dto::reference problem_reference_value{
                sample_reference_value.problem_id
            };
            const auto version_exp = db_service_util::map_repository_error_to_http_error(
                problem_core_repository::increase_version(
                    transaction,
                    problem_reference_value
                )
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<void, error_code> problem_content_service::delete_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto sample_values_exp = db_service_util::map_repository_error_to_http_error(
                problem_content_repository::list_samples(
                    transaction,
                    problem_reference_value
                )
            );
            if(!sample_values_exp){
                return std::unexpected(sample_values_exp.error());
            }
            if(sample_values_exp->empty()){
                return std::unexpected(error_code::create(http_error::not_found));
            }

            problem_content_dto::sample_ref sample_reference_value{
                .problem_id = problem_reference_value.problem_id,
                .sample_order = sample_values_exp->back().order
            };
            const auto delete_sample_exp = db_service_util::map_repository_error_to_http_error(
                problem_content_repository::delete_sample(
                    transaction,
                    sample_reference_value
                )
            );
            if(!delete_sample_exp){
                return std::unexpected(delete_sample_exp.error());
            }

            const auto version_exp = db_service_util::map_repository_error_to_http_error(
                problem_core_repository::increase_version(
                    transaction,
                    problem_reference_value
                )
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}
