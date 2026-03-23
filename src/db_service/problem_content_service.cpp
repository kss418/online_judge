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

std::expected<problem_dto::sample, error_code> problem_content_service::create_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::sample& sample_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::sample, error_code> {
            const auto created_sample_exp = problem_content_util::create_sample(
                transaction,
                problem_reference_value,
                sample_value
            );
            if(!created_sample_exp){
                return std::unexpected(created_sample_exp.error());
            }

            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return *created_sample_exp;
        }
    );
}

std::expected<problem_dto::sample, error_code> problem_content_service::get_sample(
    db_connection& connection,
    const problem_dto::sample_ref& sample_reference_value
){
    if(sample_reference_value.problem_id <= 0 || sample_reference_value.sample_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::sample, error_code> {
            return problem_content_util::get_sample(
                transaction,
                sample_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::sample>, error_code> problem_content_service::list_samples(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<void, error_code> problem_content_service::set_sample(
    db_connection& connection,
    const problem_dto::sample_ref& sample_reference_value,
    const problem_dto::sample& sample_value
){
    if(sample_reference_value.problem_id <= 0 || sample_reference_value.sample_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_sample_exp = problem_content_util::set_sample(
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

std::expected<void, error_code> problem_content_service::delete_sample(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto sample_values_exp = problem_content_util::list_samples(
                transaction,
                problem_reference_value
            );
            if(!sample_values_exp){
                return std::unexpected(sample_values_exp.error());
            }
            if(sample_values_exp->empty()){
                return std::unexpected(error_code::create(errno_error::invalid_argument));
            }

            problem_dto::sample_ref sample_reference_value{
                .problem_id = problem_reference_value.problem_id,
                .sample_order = sample_values_exp->back().order
            };
            const auto delete_sample_exp = problem_content_util::delete_sample(
                transaction,
                sample_reference_value
            );
            if(!delete_sample_exp){
                return std::unexpected(delete_sample_exp.error());
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
