#include "serializer/submission_json_serializer.hpp"

namespace{
    boost::json::object make_history_object(
        const submission_dto::history& history_value
    ){
        boost::json::object response_object;
        response_object["history_id"] = history_value.history_id;
        if(history_value.from_status_opt){
            response_object["from_status"] = *history_value.from_status_opt;
        }
        else{
            response_object["from_status"] = nullptr;
        }
        response_object["to_status"] = history_value.to_status;
        if(history_value.reason_opt){
            response_object["reason"] = *history_value.reason_opt;
        }
        else{
            response_object["reason"] = nullptr;
        }
        response_object["created_at"] = history_value.created_at;
        return response_object;
    }

    boost::json::array make_history_array(
        const submission_dto::history_list& history_values
    ){
        boost::json::array response_array;
        response_array.reserve(history_values.size());
        for(const auto& history_value : history_values){
            response_array.push_back(make_history_object(history_value));
        }
        return response_array;
    }

    boost::json::object make_summary_object(
        const submission_dto::summary& summary_value
    ){
    boost::json::object response_object;
    response_object["submission_id"] = summary_value.submission_id;
    response_object["user_id"] = summary_value.user_id;
    response_object["user_login_id"] = summary_value.user_login_id;
    response_object["problem_id"] = summary_value.problem_id;
    response_object["problem_title"] = summary_value.problem_title;
        response_object["language"] = summary_value.language;
        response_object["status"] = summary_value.status;
        response_object["created_at"] = summary_value.created_at;
        response_object["updated_at"] = summary_value.updated_at;

        if(summary_value.score_opt){
            response_object["score"] = *summary_value.score_opt;
        }
        else{
            response_object["score"] = nullptr;
        }

        if(summary_value.elapsed_ms_opt){
            response_object["elapsed_ms"] = *summary_value.elapsed_ms_opt;
        }
        else{
            response_object["elapsed_ms"] = nullptr;
        }

        if(summary_value.max_rss_kb_opt){
            response_object["max_rss_kb"] = *summary_value.max_rss_kb_opt;
        }
        else{
            response_object["max_rss_kb"] = nullptr;
        }

        if(summary_value.user_problem_state_opt){
            response_object["user_problem_state"] = *summary_value.user_problem_state_opt;
        }
        else{
            response_object["user_problem_state"] = nullptr;
        }

        return response_object;
    }

    boost::json::array make_summary_array(
        const std::vector<submission_dto::summary>& summary_values
    ){
        boost::json::array response_array;
        response_array.reserve(summary_values.size());
        for(const auto& summary_value : summary_values){
            response_array.push_back(make_summary_object(summary_value));
        }
        return response_array;
    }
}

boost::json::object submission_json_serializer::make_created_object(
    const submission_dto::created& created_value
){
    boost::json::object response_object;
    response_object["submission_id"] = created_value.submission_id;
    response_object["status"] = created_value.status;
    return response_object;
}

boost::json::object submission_json_serializer::make_history_list_object(
    std::int64_t submission_id,
    const submission_dto::history_list& history_values
){
    boost::json::object response_object;
    response_object["submission_id"] = submission_id;
    response_object["history_count"] = static_cast<std::int64_t>(history_values.size());
    response_object["histories"] = make_history_array(history_values);
    return response_object;
}

boost::json::object submission_json_serializer::make_source_object(
    const submission_dto::source_detail& source_detail_value
){
    boost::json::object response_object;
    response_object["submission_id"] = source_detail_value.submission_id;
    response_object["user_id"] = source_detail_value.user_id;
    response_object["problem_id"] = source_detail_value.problem_id;
    response_object["language"] = source_detail_value.language;
    response_object["source_code"] = source_detail_value.source_code;
    if(source_detail_value.compile_output_opt){
        response_object["compile_output"] = *source_detail_value.compile_output_opt;
    }
    else{
        response_object["compile_output"] = nullptr;
    }
    if(source_detail_value.judge_output_opt){
        response_object["judge_output"] = *source_detail_value.judge_output_opt;
    }
    else{
        response_object["judge_output"] = nullptr;
    }
    return response_object;
}

boost::json::object submission_json_serializer::make_detail_object(
    const submission_dto::detail& detail_value
){
    boost::json::object response_object;
    response_object["submission_id"] = detail_value.submission_id;
    response_object["user_id"] = detail_value.user_id;
    response_object["problem_id"] = detail_value.problem_id;
    response_object["language"] = detail_value.language;
    response_object["status"] = detail_value.status;
    response_object["created_at"] = detail_value.created_at;
    response_object["updated_at"] = detail_value.updated_at;

    if(detail_value.score_opt){
        response_object["score"] = *detail_value.score_opt;
    }
    else{
        response_object["score"] = nullptr;
    }

    if(detail_value.compile_output_opt){
        response_object["compile_output"] = *detail_value.compile_output_opt;
    }
    else{
        response_object["compile_output"] = nullptr;
    }

    if(detail_value.judge_output_opt){
        response_object["judge_output"] = *detail_value.judge_output_opt;
    }
    else{
        response_object["judge_output"] = nullptr;
    }

    if(detail_value.elapsed_ms_opt){
        response_object["elapsed_ms"] = *detail_value.elapsed_ms_opt;
    }
    else{
        response_object["elapsed_ms"] = nullptr;
    }

    if(detail_value.max_rss_kb_opt){
        response_object["max_rss_kb"] = *detail_value.max_rss_kb_opt;
    }
    else{
        response_object["max_rss_kb"] = nullptr;
    }

    return response_object;
}

boost::json::object submission_json_serializer::make_list_object(
    const std::vector<submission_dto::summary>& summary_values,
    std::int64_t total_submission_count
){
    boost::json::object response_object;
    response_object["submission_count"] = static_cast<std::int64_t>(summary_values.size());
    response_object["total_submission_count"] = total_submission_count;
    response_object["submissions"] = make_summary_array(summary_values);
    return response_object;
}
