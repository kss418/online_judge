#include "query_builder/submission_query_builder.hpp"

#include "common/language_util.hpp"
#include "db_repository/sql_filter_builder.hpp"
#include "query_builder/viewer_problem_state_sql.hpp"

#include <string>
#include <utility>

namespace{
    struct submission_list_query_context{
        const submission_dto::list_filter& filter_value;
        std::optional<std::int64_t> viewer_user_id_opt;
        sql_filter_builder predicates;
    };

    std::expected<submission_list_query_context, repository_error> make_submission_list_query_context(
        const submission_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt
    ){
        const bool is_invalid =
            (filter_value.user_id_opt && *filter_value.user_id_opt <= 0) ||
            (filter_value.user_login_id_opt && filter_value.user_login_id_opt->empty()) ||
            (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
            (
                filter_value.language_opt &&
                !language_util::find_supported_language(*filter_value.language_opt)
            ) ||
            (filter_value.limit_opt && *filter_value.limit_opt <= 0) ||
            (
                filter_value.before_submission_id_opt &&
                *filter_value.before_submission_id_opt <= 0
            ) ||
            (filter_value.status_opt && !parse_submission_status(*filter_value.status_opt)) ||
            (viewer_user_id_opt && *viewer_user_id_opt <= 0);
        if(is_invalid){
            return std::unexpected(repository_error::invalid_input);
        }

        return submission_list_query_context{
            .filter_value = filter_value,
            .viewer_user_id_opt = viewer_user_id_opt
        };
    }

    std::int32_t resolve_submission_list_limit(
        const submission_dto::list_filter& filter_value
    ){
        return filter_value.limit_opt.value_or(submission_dto::DEFAULT_LIST_LIMIT);
    }

    void append_submission_list_where_clauses(
        std::string& query,
        submission_list_query_context& context_value
    ){
        context_value.predicates.where_optional_param(
            "submission_table.user_id = ",
            context_value.filter_value.user_id_opt
        );
        context_value.predicates.where_optional_param(
            "user_table.user_login_id = ",
            context_value.filter_value.user_login_id_opt
        );
        context_value.predicates.where_optional_param(
            "submission_table.problem_id = ",
            context_value.filter_value.problem_id_opt
        );
        context_value.predicates.where_optional_param(
            "submission_table.language = ",
            context_value.filter_value.language_opt
        );
        context_value.predicates.where_optional_param_with_suffix(
            "submission_table.status = ",
            context_value.filter_value.status_opt,
            "::submission_status"
        );
        context_value.predicates.where_optional_param(
            "submission_table.submission_id < ",
            context_value.filter_value.before_submission_id_opt
        );

        query += context_value.predicates.sql();
    }

    void append_submission_list_order_by_and_limit(
        std::string& query,
        submission_list_query_context& context_value
    ){
        query +=
            " ORDER BY submission_table.submission_id DESC LIMIT " +
            context_value.predicates.append_param(
                static_cast<std::int64_t>(resolve_submission_list_limit(context_value.filter_value)) + 1
            );
    }
}

submission_query_builder::submission_list_query_builder::submission_list_query_builder(
    const submission_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
) :
    filter_value_(filter_value),
    viewer_user_id_opt_(viewer_user_id_opt){}

std::expected<assembled_query, repository_error>
submission_query_builder::submission_list_query_builder::build_list_query() const{
    auto context_exp = make_submission_list_query_context(
        filter_value_,
        viewer_user_id_opt_
    );
    if(!context_exp){
        return std::unexpected(context_exp.error());
    }
    auto context_value = std::move(*context_exp);

    std::string query =
        "SELECT "
        "submission_table.submission_id, "
        "submission_table.user_id, "
        "COALESCE(user_table.user_login_id, ''), "
        "submission_table.problem_id, "
        "problem_table.title, "
        "submission_table.language, "
        "submission_table.status::text, "
        "submission_table.score, "
        "submission_table.elapsed_ms, "
        "submission_table.max_rss_kb, " +
        viewer_problem_state_sql::make_state_select_expr(
            viewer_user_id_opt_,
            "viewer_problem_state"
        ) +
        ", "
        "submission_table.created_at::text, "
        "submission_table.updated_at::text "
        "FROM submissions submission_table "
        "JOIN problems problem_table "
        "ON problem_table.problem_id = submission_table.problem_id "
        "JOIN users user_table "
        "ON user_table.user_id = submission_table.user_id ";

    viewer_problem_state_sql::append_viewer_join(
        query,
        context_value.predicates,
        context_value.viewer_user_id_opt,
        "viewer_problem_state",
        "submission_table.problem_id"
    );
    append_submission_list_where_clauses(query, context_value);
    append_submission_list_order_by_and_limit(query, context_value);

    return assembled_query{
        .sql = std::move(query),
        .params = context_value.predicates.take_params()
    };
}

std::expected<assembled_query, repository_error>
submission_query_builder::build_status_snapshot_query(
    const std::vector<std::int64_t>& submission_ids
){
    std::string query =
        "SELECT "
        "submission_id, "
        "status::text, "
        "score, "
        "elapsed_ms, "
        "max_rss_kb "
        "FROM submissions "
        "WHERE submission_id IN (";
    pqxx::params params;

    for(std::size_t index = 0; index < submission_ids.size(); ++index){
        if(submission_ids[index] <= 0){
            return std::unexpected(repository_error::invalid_input);
        }

        if(index > 0){
            query += ", ";
        }

        query += "$" + std::to_string(index + 1);
        params.append(submission_ids[index]);
    }

    query += ") ORDER BY submission_id ASC";

    return assembled_query{
        .sql = std::move(query),
        .params = std::move(params)
    };
}
