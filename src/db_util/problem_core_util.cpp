#include "db_util/problem_core_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

std::expected<problem_dto::existence, error_code> problem_core_util::exists_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<problem_dto::title, error_code> problem_core_util::get_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<problem_dto::version, error_code> problem_core_util::get_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<problem_dto::created, error_code> problem_core_util::create_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::create_request& create_request_value
){
    if(create_request_value.title.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<std::vector<problem_dto::summary>, error_code> problem_core_util::list_problems(
    pqxx::transaction_base& transaction,
    const problem_dto::list_filter& filter_value
){
    if(filter_value.title_opt && filter_value.title_opt->empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string problem_list_query =
        "SELECT "
        "problem_id, "
        "title, "
        "version "
        "FROM problems "
        "WHERE 1 = 1";
    pqxx::params query_params;
    int query_param_index = 1;

    if(filter_value.title_opt){
        problem_list_query +=
            " AND title ILIKE $" + std::to_string(query_param_index++);
        query_params.append("%" + *filter_value.title_opt + "%");
    }

    problem_list_query += " ORDER BY problem_id DESC";

    const auto problem_summary_query = transaction.exec(
        problem_list_query,
        query_params
    );

    std::vector<problem_dto::summary> summary_values;
    summary_values.reserve(problem_summary_query.size());
    for(const auto& problem_summary_row : problem_summary_query){
        problem_dto::summary summary_value;
        summary_value.problem_id = problem_summary_row[0].as<std::int64_t>();
        summary_value.title = problem_summary_row[1].as<std::string>();
        summary_value.version = problem_summary_row[2].as<std::int32_t>();
        summary_values.push_back(std::move(summary_value));
    }

    return summary_values;
}

std::expected<problem_dto::limits, error_code> problem_core_util::get_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto limits_query_result = transaction.exec(
        "SELECT memory_limit_mb, time_limit_ms "
        "FROM problem_limits "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(limits_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::limits limits_value;
    limits_value.memory_mb = limits_query_result[0][0].as<std::int32_t>();
    limits_value.time_ms = limits_query_result[0][1].as<std::int32_t>();
    return limits_value;
}

std::expected<void, error_code> problem_core_util::set_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::limits& limits_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0 || limits_value.memory_mb <= 0 || limits_value.time_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<void, error_code> problem_core_util::increase_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
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
