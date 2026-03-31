#include "dto/submission_dto.hpp"

#include "common/language_util.hpp"
#include "common/string_util.hpp"
#include "http_core/http_util.hpp"
#include "http_core/query_param_util.hpp"

#include <pqxx/pqxx>

namespace{
    std::optional<std::int64_t> parse_non_negative_int64(std::string_view raw_value){
        if(raw_value == "0"){
            return std::int64_t{0};
        }

        return string_util::parse_positive_int64(raw_value);
    }
}

std::expected<submission_dto::source, dto_validation_error> submission_dto::make_source_from_json(
    const boost::json::object& json
){
    const auto language_opt = http_util::get_non_empty_string_field(
        json,
        "language"
    );
    if(!language_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: language",
            .field_opt = "language"
        });
    }
    if(!language_util::find_supported_language(*language_opt)){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "unsupported language: " + std::string{*language_opt},
            .field_opt = "language"
        });
    }

    const auto source_code_opt = http_util::get_non_empty_string_field(
        json,
        "source_code"
    );
    if(!source_code_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: source_code",
            .field_opt = "source_code"
        });
    }

    source source_value;
    source_value.language = std::string{*language_opt};
    source_value.source_code = std::string{*source_code_opt};
    return source_value;
}

std::expected<submission_dto::list_filter, dto_validation_error>
submission_dto::make_list_filter_from_query_params(
    const std::vector<http_util::query_param>& query_params
){
    list_filter filter_value;
    for(const auto& query_param : query_params){
        if(query_param.key == "page"){
            const auto parse_page_exp = query_param_util::parse_unique_query_param(
                filter_value.page_opt,
                query_param.key,
                query_param.value,
                string_util::parse_positive_int32
            );
            if(!parse_page_exp){
                return std::unexpected(parse_page_exp.error());
            }
            continue;
        }
        if(query_param.key == "user_id"){
            const auto parse_user_id_exp = query_param_util::parse_unique_query_param(
                filter_value.user_id_opt,
                query_param.key,
                query_param.value,
                string_util::parse_positive_int64
            );
            if(!parse_user_id_exp){
                return std::unexpected(parse_user_id_exp.error());
            }
            continue;
        }
        if(query_param.key == "user_login_id"){
            const auto parse_user_login_id_exp = query_param_util::parse_unique_query_param(
                filter_value.user_login_id_opt,
                query_param.key,
                query_param.value,
                [](std::string_view raw_value) -> std::optional<std::string> {
                    if(raw_value.empty()){
                        return std::nullopt;
                    }

                    return std::string{raw_value};
                }
            );
            if(!parse_user_login_id_exp){
                return std::unexpected(parse_user_login_id_exp.error());
            }
            continue;
        }
        if(query_param.key == "problem_id"){
            const auto parse_problem_id_exp = query_param_util::parse_unique_query_param(
                filter_value.problem_id_opt,
                query_param.key,
                query_param.value,
                string_util::parse_positive_int64
            );
            if(!parse_problem_id_exp){
                return std::unexpected(parse_problem_id_exp.error());
            }
            continue;
        }
        if(query_param.key == "language"){
            const auto parse_language_exp = query_param_util::parse_unique_query_param(
                filter_value.language_opt,
                query_param.key,
                query_param.value,
                [](std::string_view raw_value) -> std::optional<std::string> {
                    const auto language_opt = language_util::find_supported_language(raw_value);
                    if(!language_opt){
                        return std::nullopt;
                    }

                    return std::string{language_opt->language};
                }
            );
            if(!parse_language_exp){
                return std::unexpected(parse_language_exp.error());
            }
            continue;
        }
        if(query_param.key == "status"){
            const auto parse_status_exp = query_param_util::parse_unique_query_param(
                filter_value.status_opt,
                query_param.key,
                query_param.value,
                [](std::string_view raw_value) -> std::optional<std::string> {
                    const auto status_opt = parse_submission_status(raw_value);
                    if(!status_opt){
                        return std::nullopt;
                    }

                    return to_string(*status_opt);
                }
            );
            if(!parse_status_exp){
                return std::unexpected(parse_status_exp.error());
            }
            continue;
        }
        if(query_param.key == "limit"){
            const auto parse_limit_exp = query_param_util::parse_unique_query_param(
                filter_value.limit_opt,
                query_param.key,
                query_param.value,
                string_util::parse_positive_int32
            );
            if(!parse_limit_exp){
                return std::unexpected(parse_limit_exp.error());
            }
            continue;
        }
        if(query_param.key == "offset"){
            const auto parse_offset_exp = query_param_util::parse_unique_query_param(
                filter_value.offset_opt,
                query_param.key,
                query_param.value,
                parse_non_negative_int64
            );
            if(!parse_offset_exp){
                return std::unexpected(parse_offset_exp.error());
            }
            continue;
        }

        return std::unexpected(
            query_param_util::make_unsupported_query_parameter_error(query_param.key)
        );
    }

    return filter_value;
}

std::expected<submission_dto::create_request, dto_validation_error>
submission_dto::make_create_request_from_json(
    const boost::json::object& json,
    std::int64_t user_id,
    std::int64_t problem_id
){
    if(user_id <= 0){
        return std::unexpected(dto_validation_error{
            .code = "invalid_argument",
            .message = "user_id must be positive",
            .field_opt = "user_id"
        });
    }
    if(problem_id <= 0){
        return std::unexpected(dto_validation_error{
            .code = "invalid_argument",
            .message = "problem_id must be positive",
            .field_opt = "problem_id"
        });
    }

    const auto source_exp = make_source_from_json(json);
    if(!source_exp){
        return std::unexpected(source_exp.error());
    }

    create_request create_request_value;
    create_request_value.user_id = user_id;
    create_request_value.problem_id = problem_id;
    create_request_value.source_value = std::move(*source_exp);
    return create_request_value;
}

submission_dto::history submission_dto::make_history_from_row(
    const pqxx::row& submission_history_row
){
    history history_value;
    history_value.history_id = submission_history_row[0].as<std::int64_t>();
    if(!submission_history_row[1].is_null()){
        history_value.from_status_opt = submission_history_row[1].as<std::string>();
    }
    history_value.to_status = submission_history_row[2].as<std::string>();
    if(!submission_history_row[3].is_null()){
        history_value.reason_opt = submission_history_row[3].as<std::string>();
    }
    history_value.created_at = submission_history_row[4].as<std::string>();
    return history_value;
}

submission_dto::history_list submission_dto::make_history_list_from_result(
    const pqxx::result& submission_history_result
){
    history_list history_values;
    history_values.reserve(submission_history_result.size());
    for(const auto& submission_history_row : submission_history_result){
        history_values.push_back(make_history_from_row(submission_history_row));
    }
    return history_values;
}

submission_dto::source_detail submission_dto::make_source_detail_from_row(
    const pqxx::row& submission_source_row
){
    source_detail source_detail_value;
    source_detail_value.submission_id = submission_source_row[0].as<std::int64_t>();
    source_detail_value.user_id = submission_source_row[1].as<std::int64_t>();
    source_detail_value.problem_id = submission_source_row[2].as<std::int64_t>();
    source_detail_value.language = submission_source_row[3].as<std::string>();
    source_detail_value.source_code = submission_source_row[4].as<std::string>();
    if(!submission_source_row[5].is_null()){
        source_detail_value.compile_output_opt = submission_source_row[5].as<std::string>();
    }
    if(!submission_source_row[6].is_null()){
        source_detail_value.judge_output_opt = submission_source_row[6].as<std::string>();
    }
    return source_detail_value;
}

submission_dto::detail submission_dto::make_detail_from_row(
    const pqxx::row& submission_detail_row
){
    detail detail_value;
    detail_value.submission_id = submission_detail_row[0].as<std::int64_t>();
    detail_value.user_id = submission_detail_row[1].as<std::int64_t>();
    detail_value.problem_id = submission_detail_row[2].as<std::int64_t>();
    detail_value.language = submission_detail_row[3].as<std::string>();
    detail_value.status = submission_detail_row[4].as<std::string>();
    if(!submission_detail_row[5].is_null()){
        detail_value.score_opt = submission_detail_row[5].as<std::int16_t>();
    }
    if(!submission_detail_row[6].is_null()){
        detail_value.compile_output_opt = submission_detail_row[6].as<std::string>();
    }
    if(!submission_detail_row[7].is_null()){
        detail_value.judge_output_opt = submission_detail_row[7].as<std::string>();
    }
    if(!submission_detail_row[8].is_null()){
        detail_value.elapsed_ms_opt = submission_detail_row[8].as<std::int64_t>();
    }
    if(!submission_detail_row[9].is_null()){
        detail_value.max_rss_kb_opt = submission_detail_row[9].as<std::int64_t>();
    }
    detail_value.created_at = submission_detail_row[10].as<std::string>();
    detail_value.updated_at = submission_detail_row[11].as<std::string>();
    return detail_value;
}

submission_dto::created submission_dto::make_created(
    std::int64_t submission_id,
    submission_status submission_status_value
){
    created created_value;
    created_value.submission_id = submission_id;
    created_value.status = to_string(submission_status_value);
    return created_value;
}

std::expected<submission_status, error_code> submission_dto::make_submission_status(
    std::string_view submission_status_string
){
    const auto submission_status_opt = parse_submission_status(submission_status_string);
    if(!submission_status_opt){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return *submission_status_opt;
}

std::expected<submission_status, error_code> submission_dto::make_submission_status_from_row(
    const pqxx::row& submission_row,
    std::size_t status_column_index
){
    if(
        status_column_index >= submission_row.size() ||
        submission_row[status_column_index].is_null()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string submission_status_string =
        submission_row[status_column_index].as<std::string>();
    return make_submission_status(submission_status_string);
}

submission_dto::summary submission_dto::make_summary_from_row(
    const pqxx::row& submission_summary_row
){
    summary summary_value;
    summary_value.submission_id = submission_summary_row[0].as<std::int64_t>();
    summary_value.user_id = submission_summary_row[1].as<std::int64_t>();
    summary_value.user_login_id = submission_summary_row[2].as<std::string>();
    summary_value.problem_id = submission_summary_row[3].as<std::int64_t>();
    summary_value.problem_title = submission_summary_row[4].as<std::string>();
    summary_value.language = submission_summary_row[5].as<std::string>();
    summary_value.status = submission_summary_row[6].as<std::string>();
    if(!submission_summary_row[7].is_null()){
        summary_value.score_opt = submission_summary_row[7].as<std::int16_t>();
    }
    if(!submission_summary_row[8].is_null()){
        summary_value.elapsed_ms_opt = submission_summary_row[8].as<std::int64_t>();
    }
    if(!submission_summary_row[9].is_null()){
        summary_value.max_rss_kb_opt = submission_summary_row[9].as<std::int64_t>();
    }
    if(!submission_summary_row[10].is_null()){
        summary_value.user_problem_state_opt = submission_summary_row[10].as<std::string>();
    }
    summary_value.created_at = submission_summary_row[11].as<std::string>();
    summary_value.updated_at = submission_summary_row[12].as<std::string>();
    return summary_value;
}

std::vector<submission_dto::summary> submission_dto::make_summary_list_from_result(
    const pqxx::result& submission_summary_result
){
    std::vector<summary> summary_values;
    summary_values.reserve(submission_summary_result.size());
    for(const auto& submission_summary_row : submission_summary_result){
        summary_values.push_back(make_summary_from_row(submission_summary_row));
    }
    return summary_values;
}

submission_dto::queued_submission submission_dto::make_queued_submission_from_row(
    const pqxx::row& submission_queue_row
){
    queued_submission queued_submission_value;
    queued_submission_value.submission_id = submission_queue_row[0].as<std::int64_t>();
    queued_submission_value.problem_id = submission_queue_row[1].as<std::int64_t>();
    queued_submission_value.queue_wait_ms = submission_queue_row[2].as<std::int64_t>();
    queued_submission_value.language = submission_queue_row[3].as<std::string>();
    queued_submission_value.source_code = submission_queue_row[4].as<std::string>();
    return queued_submission_value;
}

submission_dto::status_update submission_dto::make_status_update(
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::string> reason_opt
){
    status_update status_update_value;
    status_update_value.submission_id = submission_id;
    status_update_value.to_status = to_status;
    status_update_value.reason_opt = std::move(reason_opt);
    return status_update_value;
}

submission_dto::finalize_request submission_dto::make_finalize_request(
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::int16_t> score_opt,
    std::optional<std::string> compile_output_opt,
    std::optional<std::string> judge_output_opt,
    std::optional<std::int64_t> elapsed_ms_opt,
    std::optional<std::int64_t> max_rss_kb_opt,
    std::optional<std::string> reason_opt
){
    finalize_request finalize_request_value;
    finalize_request_value.submission_id = submission_id;
    finalize_request_value.to_status = to_status;
    finalize_request_value.score_opt = std::move(score_opt);
    finalize_request_value.compile_output_opt = std::move(compile_output_opt);
    finalize_request_value.judge_output_opt = std::move(judge_output_opt);
    finalize_request_value.elapsed_ms_opt = std::move(elapsed_ms_opt);
    finalize_request_value.max_rss_kb_opt = std::move(max_rss_kb_opt);
    finalize_request_value.reason_opt = std::move(reason_opt);
    return finalize_request_value;
}

submission_dto::finalize_result submission_dto::make_finalize_result(
    std::int64_t problem_id,
    bool should_increase_accepted_count
){
    finalize_result finalize_result_value;
    finalize_result_value.problem_id = problem_id;
    finalize_result_value.should_increase_accepted_count = should_increase_accepted_count;
    return finalize_result_value;
}
