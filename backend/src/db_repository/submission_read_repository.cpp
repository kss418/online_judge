#include "db_repository/submission_read_repository.hpp"

#include "common/submission_status.hpp"
#include "query_builder/submission_query_builder.hpp"
#include "row_mapper/submission_row_mapper.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    std::int32_t resolve_submission_list_limit(
        const submission_request_dto::list_filter& filter_value
    ){
        return filter_value.limit_opt.value_or(submission_request_dto::DEFAULT_LIST_LIMIT);
    }
}

std::expected<submission_response_dto::history_list, repository_error>
submission_read_repository::get_submission_history(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto submission_history_query = transaction.exec(
        "SELECT "
        "history_id, "
        "from_status::text, "
        "to_status::text, "
        "reason, "
        "created_at::text "
        "FROM submission_status_history "
        "WHERE submission_id = $1 "
        "ORDER BY history_id ASC",
        pqxx::params{submission_id}
    );
    if(submission_history_query.empty()){
        return std::unexpected(repository_error::not_found);
    }

    return submission_row_mapper::map_history_result(submission_history_query);
}

std::expected<submission_response_dto::source_detail, repository_error>
submission_read_repository::get_submission_source(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto submission_source_query = transaction.exec(
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "source_code, "
        "compile_output, "
        "judge_output "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_source_query.empty()){
        return std::unexpected(repository_error::not_found);
    }

    return submission_row_mapper::map_source_detail_row(submission_source_query[0]);
}

std::expected<submission_response_dto::detail, repository_error>
submission_read_repository::get_submission_detail(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto submission_detail_result = transaction.exec(
        "SELECT "
        "submission_id, "
        "user_id, "
        "problem_id, "
        "language, "
        "status::text, "
        "score, "
        "compile_output, "
        "judge_output, "
        "elapsed_ms, "
        "max_rss_kb, "
        "created_at::text, "
        "updated_at::text "
        "FROM submissions "
        "WHERE submission_id = $1",
        pqxx::params{submission_id}
    );
    if(submission_detail_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    return submission_row_mapper::map_detail_row(submission_detail_result[0]);
}

std::expected<std::vector<submission_response_dto::status_snapshot>, repository_error>
submission_read_repository::get_submission_status_snapshots(
    pqxx::transaction_base& transaction,
    const std::vector<std::int64_t>& submission_ids
){
    if(submission_ids.empty()){
        return std::vector<submission_response_dto::status_snapshot>{};
    }

    const auto query_exp = submission_query_builder::build_status_snapshot_query(
        submission_ids
    );
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }

    const auto submission_status_result = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );
    return submission_row_mapper::map_status_snapshot_result(submission_status_result);
}

std::expected<std::vector<submission_response_dto::summary>, repository_error>
submission_read_repository::get_wa_or_ac_submissions(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto submission_summary_query = transaction.exec(
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
        "submission_table.max_rss_kb, "
        "NULL::text, "
        "submission_table.created_at::text, "
        "submission_table.updated_at::text "
        "FROM submissions submission_table "
        "JOIN problems problem_table "
        "ON problem_table.problem_id = submission_table.problem_id "
        "JOIN users user_table "
        "ON user_table.user_id = submission_table.user_id "
        "WHERE "
        "submission_table.problem_id = $1 AND "
        "("
        "submission_table.status = $2::submission_status OR "
        "submission_table.status = $3::submission_status"
        ") "
        "ORDER BY submission_table.submission_id DESC",
        pqxx::params{
            problem_id,
            to_string(submission_status::wrong_answer),
            to_string(submission_status::accepted)
        }
    );

    return submission_row_mapper::map_summary_result(submission_summary_query);
}

std::expected<submission_response_dto::summary_page, repository_error>
submission_read_repository::list_submissions(
    pqxx::transaction_base& transaction,
    const submission_request_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    auto query_exp = submission_query_builder::submission_list_query_builder{
        filter_value,
        viewer_user_id_opt
    }.build_list_query();
    if(!query_exp){
        return std::unexpected(query_exp.error());
    }

    const auto submission_summary_query = transaction.exec(
        query_exp->sql,
        std::move(query_exp->params)
    );

    auto summary_values =
        submission_row_mapper::map_summary_result(submission_summary_query);
    submission_response_dto::summary_page summary_page_value;
    summary_page_value.has_more =
        summary_values.size() >
        static_cast<std::size_t>(resolve_submission_list_limit(filter_value));
    if(summary_page_value.has_more){
        summary_values.resize(
            static_cast<std::size_t>(resolve_submission_list_limit(filter_value))
        );
        if(!summary_values.empty()){
            summary_page_value.next_before_submission_id_opt =
                summary_values.back().submission_id;
        }
    }
    summary_page_value.submissions = std::move(summary_values);
    return summary_page_value;
}
