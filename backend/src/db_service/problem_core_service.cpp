#include "db_service/problem_core_service.hpp"
#include "db_service/db_service_util.hpp"
#include "dto/problem_content_dto.hpp"
#include "db_repository/problem_content_repository.hpp"
#include "db_repository/problem_core_repository.hpp"
#include "db_repository/problem_statistics_repository.hpp"

#include <utility>

std::expected<problem_dto::existence, service_error> problem_core_service::exists_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::existence, service_error> {
            return problem_core_repository::exists_problem(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::title, service_error> problem_core_service::get_title(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::title, service_error> {
            return problem_core_repository::get_title(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<problem_dto::version, service_error> problem_core_service::get_version(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::version, service_error> {
            return problem_core_repository::get_version(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::optional<std::string>, service_error> problem_core_service::get_user_problem_state(
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
            return problem_core_repository::get_user_problem_state(
                transaction,
                problem_reference_value,
                user_id
            );
        }
    );
}

std::expected<problem_dto::detail, service_error> problem_core_service::get_problem_detail(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(
        (viewer_user_id_opt && *viewer_user_id_opt <= 0)
    ){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::detail, service_error> {
            problem_dto::detail detail_value;
            detail_value.problem_reference_value = problem_reference_value;

            const auto title_exp = problem_core_repository::get_title(
                transaction,
                problem_reference_value
            );
            if(!title_exp){
                return std::unexpected(title_exp.error());
            }
            detail_value.title_value = *title_exp;

            const auto version_exp = problem_core_repository::get_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }
            detail_value.version_value = *version_exp;

            const auto limits_exp = problem_core_repository::get_limits(
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
            if(statement_exp){
                detail_value.statement_opt = *statement_exp;
            }
            else if(statement_exp.error() != repository_error::not_found){
                return std::unexpected(statement_exp.error());
            }

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
                    problem_core_repository::get_user_problem_state(
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

std::expected<problem_dto::created, service_error> problem_core_service::create_problem(
    db_connection& connection,
    const problem_dto::create_request& create_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::created, service_error> {
            const auto created_exp = problem_core_repository::create_problem(
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

            const auto set_limits_exp = problem_core_repository::set_limits(
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

            return *created_exp;
        }
    );
}

std::expected<void, service_error> problem_core_service::update_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::update_request& update_request_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            problem_dto::title title_value;
            title_value.value = update_request_value.title;

            const auto set_title_exp = problem_core_repository::set_title(
                transaction,
                problem_reference_value,
                title_value
            );
            if(!set_title_exp){
                return std::unexpected(set_title_exp.error());
            }

            const auto version_exp = problem_core_repository::increase_version(
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

std::expected<void, service_error> problem_core_service::delete_problem(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            return problem_core_repository::delete_problem(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error> problem_core_service::list_problems(
    db_connection& connection,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::summary>, service_error> {
            return problem_core_repository::list_problems(
                transaction,
                filter_value,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::int64_t, service_error> problem_core_service::count_problems(
    db_connection& connection,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::int64_t, service_error> {
            return problem_core_repository::count_problems(
                transaction,
                filter_value,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error>
problem_core_service::list_user_solved_problems(
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
            return problem_core_repository::list_user_solved_problems(
                transaction,
                user_id,
                viewer_user_id_opt
            );
        }
    );
}

std::expected<std::vector<problem_dto::summary>, service_error>
problem_core_service::list_user_wrong_problems(
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
            return problem_core_repository::list_user_wrong_problems(
                transaction,
                user_id,
                viewer_user_id_opt
            );
        }
    );
}
