#include "query_builder/viewer_problem_state_sql.hpp"

namespace viewer_problem_state_sql{
    std::string make_state_select_expr(
        std::optional<std::int64_t> viewer_user_id_opt,
        std::string_view summary_alias
    ){
        if(!viewer_user_id_opt){
            return "NULL::TEXT";
        }

        return
            "CASE "
            "WHEN COALESCE(" + std::string(summary_alias) + ".accepted_submission_count, 0) > 0 THEN 'solved' "
            "WHEN COALESCE(" + std::string(summary_alias) + ".failed_submission_count, 0) > 0 THEN 'wrong' "
            "ELSE NULL "
            "END";
    }

    void append_viewer_join(
        std::string& query,
        sql_filter_builder& predicates,
        std::optional<std::int64_t> viewer_user_id_opt,
        std::string_view summary_alias,
        std::string_view problem_id_expr
    ){
        if(!viewer_user_id_opt){
            return;
        }

        query +=
            "LEFT JOIN user_problem_attempt_summary AS " + std::string(summary_alias) + " "
            "ON " + std::string(summary_alias) + ".problem_id = " + std::string(problem_id_expr) + " "
            "AND " + std::string(summary_alias) + ".user_id = " +
            predicates.append_param(*viewer_user_id_opt) + " ";
    }
}
