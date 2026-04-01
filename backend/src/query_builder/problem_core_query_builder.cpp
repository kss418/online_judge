#include "query_builder/problem_core_query_builder.hpp"

#include "db_repository/sql_filter_builder.hpp"
#include "query_builder/viewer_problem_state_sql.hpp"

#include <utility>

namespace{
    struct problem_list_query_context{
        const problem_dto::list_filter& filter_value;
        std::optional<std::int64_t> viewer_user_id_opt;
        sql_filter_builder predicates;
    };

    std::expected<problem_list_query_context, error_code> make_problem_list_query_context(
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt
    ){
        const bool is_invalid =
            (filter_value.title_opt && filter_value.title_opt->empty()) ||
            (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
            (filter_value.limit_opt && *filter_value.limit_opt <= 0) ||
            (filter_value.offset_opt && *filter_value.offset_opt < 0) ||
            (filter_value.state_opt && !viewer_user_id_opt) ||
            (viewer_user_id_opt && *viewer_user_id_opt <= 0);
        if(is_invalid){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return problem_list_query_context{
            .filter_value = filter_value,
            .viewer_user_id_opt = viewer_user_id_opt
        };
    }

    std::string resolve_problem_list_sort_direction(
        const problem_dto::list_filter& filter_value
    ){
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

    void append_problem_list_where_clauses(
        std::string& query,
        problem_list_query_context& context_value
    ){
        context_value.predicates.where_optional_param(
            "p.problem_id = ",
            context_value.filter_value.problem_id_opt
        );

        if(context_value.filter_value.title_opt){
            context_value.predicates.where_param(
                "p.title ILIKE ",
                "%" + *context_value.filter_value.title_opt + "%"
            );
        }

        if(context_value.filter_value.state_opt){
            if(*context_value.filter_value.state_opt == "solved"){
                context_value.predicates.where("COALESCE(ups.accepted_submission_count, 0) > 0");
            }
            else if(*context_value.filter_value.state_opt == "unsolved"){
                context_value.predicates.where("COALESCE(ups.accepted_submission_count, 0) = 0");
            }
        }

        query += context_value.predicates.sql();
    }

    void append_problem_list_order_by(
        std::string& query,
        const problem_dto::list_filter& filter_value
    ){
        const std::string sort_direction = resolve_problem_list_sort_direction(filter_value);
        if(!filter_value.sort_opt){
            query += " ORDER BY p.problem_id DESC";
        }
        else if(*filter_value.sort_opt == "problem_id"){
            query += " ORDER BY p.problem_id " + sort_direction;
        }
        else if(*filter_value.sort_opt == "accepted_count"){
            query +=
                " ORDER BY COALESCE(ps.accepted_count, 0) " + sort_direction +
                ", p.problem_id ASC";
        }
        else if(*filter_value.sort_opt == "submission_count"){
            query +=
                " ORDER BY COALESCE(ps.submission_count, 0) " + sort_direction +
                ", p.problem_id ASC";
        }
        else if(*filter_value.sort_opt == "acceptance_rate"){
            if(sort_direction == "asc"){
                query +=
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
                query +=
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
    }

    void append_problem_list_limit_offset(
        std::string& query,
        problem_list_query_context& context_value
    ){
        if(context_value.filter_value.limit_opt){
            query +=
                " LIMIT " +
                context_value.predicates.append_param(*context_value.filter_value.limit_opt);
        }

        if(context_value.filter_value.offset_opt){
            query +=
                " OFFSET " +
                context_value.predicates.append_param(*context_value.filter_value.offset_opt);
        }
    }

    std::expected<assembled_query, error_code> build_problem_list_query(
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt,
        bool include_limit_offset,
        bool count_only
    ){
        auto context_exp = make_problem_list_query_context(
            filter_value,
            viewer_user_id_opt
        );
        if(!context_exp){
            return std::unexpected(context_exp.error());
        }
        auto context_value = std::move(*context_exp);

        std::string query;
        if(count_only){
            query =
                "SELECT COUNT(*) "
                "FROM problems AS p ";
        }
        else{
            query =
                "SELECT "
                "p.problem_id, "
                "p.title, "
                "p.version, "
                "COALESCE(pl.time_limit_ms, 0), "
                "COALESCE(pl.memory_limit_mb, 0), "
                "COALESCE(ps.submission_count, 0), "
                "COALESCE(ps.accepted_count, 0), " +
                viewer_problem_state_sql::make_state_select_expr(viewer_user_id_opt, "ups") +
                " "
                "FROM problems AS p "
                "LEFT JOIN problem_limits AS pl "
                "ON pl.problem_id = p.problem_id "
                "LEFT JOIN problem_statistics AS ps "
                "ON ps.problem_id = p.problem_id ";
        }

        viewer_problem_state_sql::append_viewer_join(
            query,
            context_value.predicates,
            context_value.viewer_user_id_opt,
            "ups",
            "p.problem_id"
        );
        append_problem_list_where_clauses(query, context_value);

        if(!count_only){
            append_problem_list_order_by(query, context_value.filter_value);
        }
        if(include_limit_offset){
            append_problem_list_limit_offset(query, context_value);
        }

        return assembled_query{
            .sql = std::move(query),
            .params = context_value.predicates.take_params()
        };
    }

    struct user_problem_list_query_context{
        std::int64_t user_id = 0;
        std::optional<std::int64_t> viewer_user_id_opt;
        sql_filter_builder predicates;
    };

    std::expected<user_problem_list_query_context, error_code> make_user_problem_list_query_context(
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt
    ){
        if(user_id <= 0 || (viewer_user_id_opt && *viewer_user_id_opt <= 0)){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return user_problem_list_query_context{
            .user_id = user_id,
            .viewer_user_id_opt = viewer_user_id_opt
        };
    }

    void append_user_problem_list_where_clauses(
        std::string& query,
        user_problem_list_query_context& context_value
    ){
        context_value.predicates.where_param("target_ups.user_id = ", context_value.user_id);
        query += context_value.predicates.sql();
    }

    std::expected<assembled_query, error_code>
    build_user_problem_list_query(
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt,
        bool solved_only
    ){
        auto context_exp = make_user_problem_list_query_context(user_id, viewer_user_id_opt);
        if(!context_exp){
            return std::unexpected(context_exp.error());
        }
        auto context_value = std::move(*context_exp);

        std::string query =
            "SELECT "
            "p.problem_id, "
            "p.title, "
            "p.version, "
            "COALESCE(pl.time_limit_ms, 0), "
            "COALESCE(pl.memory_limit_mb, 0), "
            "COALESCE(ps.submission_count, 0), "
            "COALESCE(ps.accepted_count, 0), " +
            viewer_problem_state_sql::make_state_select_expr(
                context_value.viewer_user_id_opt,
                "viewer_ups"
            ) +
            " "
            "FROM user_problem_attempt_summary AS target_ups "
            "JOIN problems AS p "
            "ON p.problem_id = target_ups.problem_id "
            "LEFT JOIN problem_limits AS pl "
            "ON pl.problem_id = p.problem_id "
            "LEFT JOIN problem_statistics AS ps "
            "ON ps.problem_id = p.problem_id ";

        viewer_problem_state_sql::append_viewer_join(
            query,
            context_value.predicates,
            context_value.viewer_user_id_opt,
            "viewer_ups",
            "p.problem_id"
        );
        if(solved_only){
            context_value.predicates.where("target_ups.accepted_submission_count > 0");
        }
        else{
            context_value.predicates.where("COALESCE(target_ups.accepted_submission_count, 0) = 0");
            context_value.predicates.where("COALESCE(target_ups.failed_submission_count, 0) > 0");
        }
        append_user_problem_list_where_clauses(query, context_value);
        query += " ORDER BY p.problem_id DESC";

        return assembled_query{
            .sql = std::move(query),
            .params = context_value.predicates.take_params()
        };
    }
}

problem_core_query_builder::problem_list_query_builder::problem_list_query_builder(
    const problem_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
) :
    filter_value_(filter_value),
    viewer_user_id_opt_(viewer_user_id_opt){}

std::expected<assembled_query, error_code>
problem_core_query_builder::problem_list_query_builder::build_list_query() const{
    return build_problem_list_query(
        filter_value_,
        viewer_user_id_opt_,
        true,
        false
    );
}

std::expected<assembled_query, error_code>
problem_core_query_builder::problem_list_query_builder::build_count_query() const{
    return build_problem_list_query(
        filter_value_,
        viewer_user_id_opt_,
        false,
        true
    );
}

problem_core_query_builder::user_problem_list_query_builder::user_problem_list_query_builder(
    std::int64_t user_id,
    std::optional<std::int64_t> viewer_user_id_opt
) :
    user_id_(user_id),
    viewer_user_id_opt_(viewer_user_id_opt){}

std::expected<assembled_query, error_code>
problem_core_query_builder::user_problem_list_query_builder::build_solved_query() const{
    return build_user_problem_list_query(
        user_id_,
        viewer_user_id_opt_,
        true
    );
}

std::expected<assembled_query, error_code>
problem_core_query_builder::user_problem_list_query_builder::build_wrong_query() const{
    return build_user_problem_list_query(
        user_id_,
        viewer_user_id_opt_,
        false
    );
}
