#include "db_repository/problem_core_repository.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

namespace{
    std::string resolve_problem_list_sort_direction(const problem_dto::list_filter& filter_value){
        if(filter_value.direction_opt){
            return *filter_value.direction_opt;
        }

        if(!filter_value.sort_opt){
            return "desc";
        }

        if(*filter_value.sort_opt == "problem_id"){
            return "asc";
        }

        return "desc";
    }

    bool has_invalid_problem_list_filter(
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt
    ){
        return
            (filter_value.title_opt && filter_value.title_opt->empty()) ||
            (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
            (filter_value.limit_opt && *filter_value.limit_opt <= 0) ||
            (filter_value.offset_opt && *filter_value.offset_opt < 0) ||
            (filter_value.state_opt && !viewer_user_id_opt) ||
            (viewer_user_id_opt && *viewer_user_id_opt <= 0);
    }

    void append_problem_list_filter_clauses(
        std::string& query,
        pqxx::params& query_params,
        int& query_param_index,
        const problem_dto::list_filter& filter_value
    ){
        query += "WHERE 1 = 1";

        if(filter_value.title_opt){
            query += " AND p.title ILIKE $" + std::to_string(query_param_index++);
            query_params.append("%" + *filter_value.title_opt + "%");
        }

        if(filter_value.problem_id_opt){
            query += " AND p.problem_id = $" + std::to_string(query_param_index++);
            query_params.append(*filter_value.problem_id_opt);
        }

        if(filter_value.state_opt){
            if(*filter_value.state_opt == "solved"){
                query += " AND COALESCE(ups.accepted_submission_count, 0) > 0";
            }
            else if(*filter_value.state_opt == "unsolved"){
                query += " AND COALESCE(ups.accepted_submission_count, 0) = 0";
            }
        }
    }
}

std::expected<problem_dto::existence, error_code> problem_core_repository::exists_problem(
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

std::expected<problem_dto::title, error_code> problem_core_repository::get_title(
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

std::expected<problem_dto::version, error_code> problem_core_repository::get_version(
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

std::expected<std::optional<std::string>, error_code> problem_core_repository::get_user_problem_state(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    std::int64_t user_id
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0 || user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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

std::expected<void, error_code> problem_core_repository::set_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::title& title_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0 || title_value.value.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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
    if(has_invalid_problem_list_filter(filter_value, viewer_user_id_opt)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string problem_list_query =
        "SELECT "
        "p.problem_id, "
        "p.title, "
        "p.version, "
        "COALESCE(pl.time_limit_ms, 0), "
        "COALESCE(pl.memory_limit_mb, 0), "
        "COALESCE(ps.submission_count, 0), "
        "COALESCE(ps.accepted_count, 0), ";
    pqxx::params query_params;
    int query_param_index = 1;

    if(viewer_user_id_opt){
        problem_list_query +=
            "CASE "
            "WHEN COALESCE(ups.accepted_submission_count, 0) > 0 THEN 'solved' "
            "WHEN COALESCE(ups.failed_submission_count, 0) > 0 THEN 'wrong' "
            "ELSE NULL "
            "END ";
    }
    else{
        problem_list_query += "NULL::TEXT ";
    }

    problem_list_query +=
        "FROM problems AS p "
        "LEFT JOIN problem_limits AS pl "
        "ON pl.problem_id = p.problem_id "
        "LEFT JOIN problem_statistics AS ps "
        "ON ps.problem_id = p.problem_id ";

    if(viewer_user_id_opt){
        problem_list_query +=
            "LEFT JOIN user_problem_attempt_summary AS ups "
            "ON ups.problem_id = p.problem_id "
            "AND ups.user_id = $" + std::to_string(query_param_index++) + " ";
        query_params.append(*viewer_user_id_opt);
    }

    append_problem_list_filter_clauses(
        problem_list_query,
        query_params,
        query_param_index,
        filter_value
    );

    const std::string sort_direction = resolve_problem_list_sort_direction(filter_value);
    if(!filter_value.sort_opt){
        problem_list_query += " ORDER BY p.problem_id DESC";
    }
    else if(*filter_value.sort_opt == "problem_id"){
        problem_list_query += " ORDER BY p.problem_id " + sort_direction;
    }
    else if(*filter_value.sort_opt == "accepted_count"){
        problem_list_query +=
            " ORDER BY COALESCE(ps.accepted_count, 0) " + sort_direction +
            ", p.problem_id ASC";
    }
    else if(*filter_value.sort_opt == "submission_count"){
        problem_list_query +=
            " ORDER BY COALESCE(ps.submission_count, 0) " + sort_direction +
            ", p.problem_id ASC";
    }
    else if(*filter_value.sort_opt == "acceptance_rate"){
        if(sort_direction == "asc"){
            problem_list_query +=
                " ORDER BY "
                "CASE WHEN COALESCE(ps.submission_count, 0) > 0 THEN 1 ELSE 0 END ASC, "
                "COALESCE("
                "COALESCE(ps.accepted_count, 0)::DOUBLE PRECISION / "
                "NULLIF(COALESCE(ps.submission_count, 0), 0), "
                "0.0"
                ") ASC, "
                "p.problem_id ASC";
        }
        else{
            problem_list_query +=
                " ORDER BY "
                "CASE WHEN COALESCE(ps.submission_count, 0) > 0 THEN 0 ELSE 1 END ASC, "
                "COALESCE("
                "COALESCE(ps.accepted_count, 0)::DOUBLE PRECISION / "
                "NULLIF(COALESCE(ps.submission_count, 0), 0), "
                "0.0"
                ") DESC, "
                "p.problem_id ASC";
        }
    }

    if(filter_value.limit_opt){
        problem_list_query += " LIMIT $" + std::to_string(query_param_index++);
        query_params.append(*filter_value.limit_opt);
    }

    if(filter_value.offset_opt){
        problem_list_query += " OFFSET $" + std::to_string(query_param_index++);
        query_params.append(*filter_value.offset_opt);
    }

    const auto problem_summary_query = transaction.exec(
        problem_list_query,
        query_params
    );

    return problem_dto::make_summary_list_from_result(problem_summary_query);
}

std::expected<std::int64_t, error_code> problem_core_repository::count_problems(
    pqxx::transaction_base& transaction,
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(has_invalid_problem_list_filter(filter_value, viewer_user_id_opt)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string problem_count_query =
        "SELECT COUNT(*) "
        "FROM problems AS p ";
    pqxx::params query_params;
    int query_param_index = 1;

    if(viewer_user_id_opt){
        problem_count_query +=
            "LEFT JOIN user_problem_attempt_summary AS ups "
            "ON ups.problem_id = p.problem_id "
            "AND ups.user_id = $" + std::to_string(query_param_index++) + " ";
        query_params.append(*viewer_user_id_opt);
    }

    append_problem_list_filter_clauses(
        problem_count_query,
        query_params,
        query_param_index,
        filter_value
    );

    const auto problem_count_result = transaction.exec(
        problem_count_query,
        query_params
    );
    if(problem_count_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return problem_count_result[0][0].as<std::int64_t>();
}

std::expected<std::vector<problem_dto::summary>, error_code>
problem_core_repository::list_user_solved_problems(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(user_id <= 0 || (viewer_user_id_opt && *viewer_user_id_opt <= 0)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string problem_list_query =
        "SELECT "
        "p.problem_id, "
        "p.title, "
        "p.version, "
        "COALESCE(pl.time_limit_ms, 0), "
        "COALESCE(pl.memory_limit_mb, 0), "
        "COALESCE(ps.submission_count, 0), "
        "COALESCE(ps.accepted_count, 0), ";
    pqxx::params query_params;
    int query_param_index = 1;

    if(viewer_user_id_opt){
        problem_list_query +=
            "CASE "
            "WHEN COALESCE(viewer_ups.accepted_submission_count, 0) > 0 THEN 'solved' "
            "WHEN COALESCE(viewer_ups.failed_submission_count, 0) > 0 THEN 'wrong' "
            "ELSE NULL "
            "END ";
    }
    else{
        problem_list_query += "NULL::TEXT ";
    }

    problem_list_query +=
        "FROM user_problem_attempt_summary AS target_ups "
        "JOIN problems AS p "
        "ON p.problem_id = target_ups.problem_id "
        "LEFT JOIN problem_limits AS pl "
        "ON pl.problem_id = p.problem_id "
        "LEFT JOIN problem_statistics AS ps "
        "ON ps.problem_id = p.problem_id ";

    query_params.append(user_id);
    if(viewer_user_id_opt){
        problem_list_query +=
            "LEFT JOIN user_problem_attempt_summary AS viewer_ups "
            "ON viewer_ups.problem_id = p.problem_id "
            "AND viewer_ups.user_id = $" + std::to_string(++query_param_index) + " ";
        query_params.append(*viewer_user_id_opt);
    }

    problem_list_query +=
        "WHERE target_ups.user_id = $1 "
        "AND target_ups.accepted_submission_count > 0 "
        "ORDER BY p.problem_id DESC";

    const auto problem_summary_query = transaction.exec(
        problem_list_query,
        query_params
    );

    return problem_dto::make_summary_list_from_result(problem_summary_query);
}

std::expected<std::vector<problem_dto::summary>, error_code>
problem_core_repository::list_user_wrong_problems(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
){
    if(user_id <= 0 || (viewer_user_id_opt && *viewer_user_id_opt <= 0)){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string problem_list_query =
        "SELECT "
        "p.problem_id, "
        "p.title, "
        "p.version, "
        "COALESCE(pl.time_limit_ms, 0), "
        "COALESCE(pl.memory_limit_mb, 0), "
        "COALESCE(ps.submission_count, 0), "
        "COALESCE(ps.accepted_count, 0), ";
    pqxx::params query_params;
    int query_param_index = 1;

    if(viewer_user_id_opt){
        problem_list_query +=
            "CASE "
            "WHEN COALESCE(viewer_ups.accepted_submission_count, 0) > 0 THEN 'solved' "
            "WHEN COALESCE(viewer_ups.failed_submission_count, 0) > 0 THEN 'wrong' "
            "ELSE NULL "
            "END ";
    }
    else{
        problem_list_query += "NULL::TEXT ";
    }

    problem_list_query +=
        "FROM user_problem_attempt_summary AS target_ups "
        "JOIN problems AS p "
        "ON p.problem_id = target_ups.problem_id "
        "LEFT JOIN problem_limits AS pl "
        "ON pl.problem_id = p.problem_id "
        "LEFT JOIN problem_statistics AS ps "
        "ON ps.problem_id = p.problem_id ";

    query_params.append(user_id);
    if(viewer_user_id_opt){
        problem_list_query +=
            "LEFT JOIN user_problem_attempt_summary AS viewer_ups "
            "ON viewer_ups.problem_id = p.problem_id "
            "AND viewer_ups.user_id = $" + std::to_string(++query_param_index) + " ";
        query_params.append(*viewer_user_id_opt);
    }

    problem_list_query +=
        "WHERE target_ups.user_id = $1 "
        "AND COALESCE(target_ups.accepted_submission_count, 0) = 0 "
        "AND COALESCE(target_ups.failed_submission_count, 0) > 0 "
        "ORDER BY p.problem_id DESC";

    const auto problem_summary_query = transaction.exec(
        problem_list_query,
        query_params
    );

    return problem_dto::make_summary_list_from_result(problem_summary_query);
}

std::expected<problem_content_dto::limits, error_code> problem_core_repository::get_limits(
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

std::expected<void, error_code> problem_core_repository::increase_version(
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
