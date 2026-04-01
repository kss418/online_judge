#include "db_repository/problem_core_repository.hpp"
#include "query_builder/problem_core_query_builder.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    std::expected<std::vector<problem_dto::summary>, error_code> map_problem_summary_rows(
        const pqxx::result& query_result
    ){
        return problem_dto::make_summary_list_from_result(query_result);
    }

    std::expected<std::int64_t, error_code> map_problem_count_row(
        const pqxx::result& query_result
    ){
        if(query_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        return query_result[0][0].as<std::int64_t>();
    }
}

std::expected<problem_dto::existence, error_code> problem_core_repository::exists_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto exists_query_result = transaction.exec(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM problems "
        "WHERE problem_id = $1"
        ")",
        pqxx::params{problem_id}
    );

    if(exists_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::existence existence_value;
    existence_value.exists = exists_query_result[0][0].as<bool>();
    return existence_value;
}

std::expected<problem_dto::title, error_code> problem_core_repository::get_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto title_query_result = transaction.exec(
        "SELECT title "
        "FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(title_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::title title_value;
    title_value.value = title_query_result[0][0].as<std::string>();
    return title_value;
}

std::expected<problem_dto::version, error_code> problem_core_repository::get_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto version_query_result = transaction.exec(
        "SELECT version "
        "FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(version_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::version version_value;
    version_value.version = version_query_result[0][0].as<std::int32_t>();
    return version_value;
}

std::expected<std::optional<std::string>, error_code> problem_core_repository::get_user_problem_state(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    std::int64_t user_id
){
    if(!problem_dto::is_valid(problem_reference_value) || user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto state_query_result = transaction.exec(
        "SELECT accepted_submission_count, failed_submission_count "
        "FROM user_problem_attempt_summary "
        "WHERE user_id = $1 AND problem_id = $2",
        pqxx::params{user_id, problem_id}
    );

    if(state_query_result.empty()){
        return std::optional<std::string>{};
    }

    const std::int64_t accepted_submission_count =
        state_query_result[0][0].as<std::int64_t>();
    const std::int64_t failed_submission_count =
        state_query_result[0][1].as<std::int64_t>();

    if(accepted_submission_count > 0){
        return std::optional<std::string>{"solved"};
    }
    if(failed_submission_count > 0){
        return std::optional<std::string>{"wrong"};
    }

    return std::optional<std::string>{};
}

std::expected<problem_dto::created, error_code> problem_core_repository::create_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::create_request& create_request_value
){
    const auto create_problem_result = transaction.exec(
        "INSERT INTO problems(version, title) "
        "VALUES($1, $2) "
        "RETURNING problem_id",
        pqxx::params{1, create_request_value.title}
    );

    if(create_problem_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::created created_value;
    created_value.problem_id = create_problem_result[0][0].as<std::int64_t>();
    return created_value;
}

std::expected<void, error_code> problem_core_repository::set_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::title& title_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET title = $2 "
        "WHERE problem_id = $1",
        pqxx::params{problem_id, title_value.value}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> problem_core_repository::delete_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto delete_result = transaction.exec(
        "DELETE FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<std::vector<problem_dto::summary>, error_code> problem_core_repository::list_problems(
    pqxx::transaction_base& transaction,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = problem_core_query_builder::problem_list_query_builder{
        filter_value,
        viewer_user_id_opt
    }.build_list_query();
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }
    const auto problem_summary_query = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );

    return map_problem_summary_rows(problem_summary_query);
}

std::expected<std::int64_t, error_code> problem_core_repository::count_problems(
    pqxx::transaction_base& transaction,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = problem_core_query_builder::problem_list_query_builder{
        filter_value,
        viewer_user_id_opt
    }.build_count_query();
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }
    const auto problem_count_result = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );
    return map_problem_count_row(problem_count_result);
}

std::expected<std::vector<problem_dto::summary>, error_code>
problem_core_repository::list_user_solved_problems(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = problem_core_query_builder::user_problem_list_query_builder{
        user_id,
        viewer_user_id_opt
    }.build_solved_query();
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }
    const auto problem_summary_query = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );

    return map_problem_summary_rows(problem_summary_query);
}

std::expected<std::vector<problem_dto::summary>, error_code>
problem_core_repository::list_user_wrong_problems(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = problem_core_query_builder::user_problem_list_query_builder{
        user_id,
        viewer_user_id_opt
    }.build_wrong_query();
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }
    const auto problem_summary_query = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );

    return map_problem_summary_rows(problem_summary_query);
}

std::expected<problem_content_dto::limits, error_code> problem_core_repository::get_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto limits_query_result = transaction.exec(
        "SELECT memory_limit_mb, time_limit_ms "
        "FROM problem_limits "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(limits_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_content_dto::limits limits_value;
    limits_value.memory_mb = limits_query_result[0][0].as<std::int32_t>();
    limits_value.time_ms = limits_query_result[0][1].as<std::int32_t>();
    return limits_value;
}

std::expected<void, error_code> problem_core_repository::set_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::limits& limits_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    transaction.exec(
        "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
        "VALUES($1, $2, $3, NOW()) "
        "ON CONFLICT(problem_id) DO UPDATE "
        "SET "
        "memory_limit_mb = EXCLUDED.memory_limit_mb, "
        "time_limit_ms = EXCLUDED.time_limit_ms, "
        "updated_at = NOW()",
        pqxx::params{
            problem_id,
            limits_value.memory_mb,
            limits_value.time_ms
        }
    );

    return {};
}

std::expected<void, error_code> problem_core_repository::increase_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::int64_t problem_id = problem_reference_value.problem_id;
    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
