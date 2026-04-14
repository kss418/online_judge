#include "db_repository/problem_query_repository.hpp"

#include "query_builder/problem_core_query_builder.hpp"
#include "row_mapper/problem_row_mapper.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    std::expected<std::vector<problem_dto::summary>, repository_error> map_problem_summary_rows(
        const pqxx::result& query_result
    ){
        return problem_row_mapper::map_summary_result(query_result);
    }

    std::expected<std::int64_t, repository_error> map_problem_count_row(
        const pqxx::result& query_result
    ){
        if(query_result.empty()){
            return std::unexpected(repository_error::internal);
        }

        return query_result[0][0].as<std::int64_t>();
    }
}

std::expected<problem_dto::existence, repository_error> problem_query_repository::exists_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
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
        return std::unexpected(repository_error::internal);
    }

    problem_dto::existence existence_value;
    existence_value.exists = exists_query_result[0][0].as<bool>();
    return existence_value;
}

std::expected<problem_dto::title, repository_error> problem_query_repository::get_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto title_query_result = transaction.exec(
        "SELECT title "
        "FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(title_query_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::title title_value;
    title_value.value = title_query_result[0][0].as<std::string>();
    return title_value;
}

std::expected<problem_dto::version, repository_error> problem_query_repository::get_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto version_query_result = transaction.exec(
        "SELECT version "
        "FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(version_query_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::version version_value;
    version_value.version = version_query_result[0][0].as<std::int32_t>();
    return version_value;
}

std::expected<std::optional<std::string>, repository_error>
problem_query_repository::get_user_problem_state(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    std::int64_t user_id
){
    if(!problem_dto::is_valid(problem_reference_value) || user_id <= 0){
        return std::unexpected(repository_error::invalid_input);
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

std::expected<std::vector<problem_dto::summary>, repository_error>
problem_query_repository::list_problems(
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

std::expected<std::int64_t, repository_error> problem_query_repository::count_problems(
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

std::expected<std::vector<problem_dto::summary>, repository_error>
problem_query_repository::list_user_solved_problems(
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

std::expected<std::vector<problem_dto::summary>, repository_error>
problem_query_repository::list_user_wrong_problems(
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

std::expected<problem_content_dto::limits, repository_error>
problem_query_repository::get_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto limits_query_result = transaction.exec(
        "SELECT memory_limit_mb, time_limit_ms "
        "FROM problem_limits "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(limits_query_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_content_dto::limits limits_value;
    limits_value.memory_mb = limits_query_result[0][0].as<std::int32_t>();
    limits_value.time_ms = limits_query_result[0][1].as<std::int32_t>();
    return limits_value;
}
