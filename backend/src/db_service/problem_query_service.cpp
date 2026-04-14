#include "db_service/problem_query_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_repository/problem_content_repository.hpp"
#include "db_repository/problem_query_repository.hpp"
#include "db_repository/problem_statistics_repository.hpp"

#include <utility>

std::expected<void, service_error> problem_query_service::ensure_problem_exists(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<void, service_error> {
            const auto existence_exp = problem_query_repository::exists_problem(
                transaction,
                problem_reference_value
            );
            if(!existence_exp){
                return std::unexpected(existence_exp.error());
            }
            if(!existence_exp->exists){
                return std::unexpected(service_error::not_found);
            }

            return {};
        }
    );
}

std::expected<problem_dto::title, service_error> problem_query_service::get_title(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::title, service_error> {
            return problem_query_repository::get_title(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::version, service_error> problem_query_service::get_version(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::version, service_error> {
            return problem_query_repository::get_version(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::optional<std::string>, service_error>
problem_query_service::get_user_problem_state(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::optional<std::string>, service_error> {
            return problem_query_repository::get_user_problem_state(
                transaction,
                problem_reference_value,
                user_id
            );
        }
    );
}

std::expected<problem_dto::detail, service_error> problem_query_service::get_problem_detail(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(viewer_user_id_opt && *viewer_user_id_opt <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::detail, service_error> {
            problem_dto::detail detail_value;
            detail_value.problem_reference_value = problem_reference_value;

            const auto title_exp = problem_query_repository::get_title(
                transaction,
                problem_reference_value
            );
            if(!title_exp){
                return std::unexpected(title_exp.error());
            }
            detail_value.title_value = *title_exp;

            const auto version_exp = problem_query_repository::get_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }
            detail_value.version_value = *version_exp;

            const auto limits_exp = problem_query_repository::get_limits(
                transaction,
                problem_reference_value
            );
            if(!limits_exp){
                return std::unexpected(limits_exp.error());
            }
            detail_value.limits_value = *limits_exp;

            const auto statement_exp = problem_content_repository::get_statement(
                transaction,
                problem_reference_value
            );
            if(!statement_exp){
                return std::unexpected(statement_exp.error());
            }
            detail_value.statement_opt = std::move(*statement_exp);

            const auto sample_values_exp = problem_content_repository::list_samples(
                transaction,
                problem_reference_value
            );
            if(!sample_values_exp){
                return std::unexpected(sample_values_exp.error());
            }
            detail_value.sample_values = std::move(*sample_values_exp);

            const auto statistics_exp = problem_statistics_repository::get_statistics(
                transaction,
                problem_reference_value
            );
            if(!statistics_exp){
                return std::unexpected(statistics_exp.error());
            }
            detail_value.statistics_value = *statistics_exp;

            if(viewer_user_id_opt){
                const auto user_problem_state_exp =
                    problem_query_repository::get_user_problem_state(
                        transaction,
                        problem_reference_value,
                        *viewer_user_id_opt
                    );
                if(!user_problem_state_exp){
                    return std::unexpected(user_problem_state_exp.error());
                }
                detail_value.user_problem_state_opt = *user_problem_state_exp;
            }

            return detail_value;
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error>
problem_query_service::list_problems(
    db_connection& connection,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::summary>, service_error> {
            return problem_query_repository::list_problems(
                transaction,
                filter_value,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::int64_t, service_error> problem_query_service::count_problems(
    db_connection& connection,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::int64_t, service_error> {
            return problem_query_repository::count_problems(
                transaction,
                filter_value,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error>
problem_query_service::list_user_solved_problems(
    db_connection& connection,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(user_id <= 0 || (viewer_user_id_opt && *viewer_user_id_opt <= 0)){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::summary>, service_error> {
            return problem_query_repository::list_user_solved_problems(
                transaction,
                user_id,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error>
problem_query_service::list_user_wrong_problems(
    db_connection& connection,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(user_id <= 0 || (viewer_user_id_opt && *viewer_user_id_opt <= 0)){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::summary>, service_error> {
            return problem_query_repository::list_user_wrong_problems(
                transaction,
                user_id,
                viewer_user_id_opt
            );
        }
    );
}
