#include "http_server/json_util.hpp"

#include <utility>

boost::json::object json_util::make_error_object(
    std::string_view code,
    std::string_view message,
    std::optional<std::string> field_opt
){
    boost::json::object error_object;
    error_object["code"] = code;
    error_object["message"] = message;
    if(field_opt){
        error_object["field"] = *field_opt;
    }

    boost::json::object response_object;
    response_object["error"] = std::move(error_object);
    return response_object;
}

boost::json::object json_util::make_message_object(std::string_view message){
    boost::json::object response_object;
    response_object["message"] = message;
    return response_object;
}

boost::json::object json_util::make_auth_session_object(const auth_dto::session& session_value){
    boost::json::object response_object;
    response_object["user_id"] = session_value.user_id;
    response_object["is_admin"] = session_value.is_admin;
    response_object["token"] = session_value.token;
    return response_object;
}

boost::json::object json_util::make_submission_created_object(
    const submission_dto::created& created_value
){
    boost::json::object response_object;
    response_object["submission_id"] = created_value.submission_id;
    response_object["status"] = created_value.status;
    return response_object;
}

boost::json::object json_util::make_submission_summary_object(
    const submission_dto::summary& summary_value
){
    boost::json::object response_object;
    response_object["submission_id"] = summary_value.submission_id;
    response_object["user_id"] = summary_value.user_id;
    response_object["problem_id"] = summary_value.problem_id;
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

    return response_object;
}

boost::json::object json_util::make_submission_detail_object(
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

boost::json::array json_util::make_submission_summary_array(
    const std::vector<submission_dto::summary>& summary_values
){
    boost::json::array response_array;
    response_array.reserve(summary_values.size());
    for(const auto& summary_value : summary_values){
        response_array.push_back(make_submission_summary_object(summary_value));
    }
    return response_array;
}

boost::json::object json_util::make_submission_list_object(
    const std::vector<submission_dto::summary>& summary_values
){
    boost::json::object response_object;
    response_object["submission_count"] = static_cast<std::int64_t>(summary_values.size());
    response_object["submissions"] = make_submission_summary_array(summary_values);
    return response_object;
}

boost::json::object json_util::make_problem_created_object(
    const problem_dto::created& created_value
){
    boost::json::object response_object;
    response_object["problem_id"] = created_value.problem_id;
    return response_object;
}

boost::json::object json_util::make_problem_summary_object(
    const problem_dto::summary& summary_value
){
    boost::json::object response_object;
    response_object["problem_id"] = summary_value.problem_id;
    response_object["title"] = summary_value.title;
    response_object["version"] = summary_value.version;
    return response_object;
}

boost::json::array json_util::make_problem_summary_array(
    const std::vector<problem_dto::summary>& summary_values
){
    boost::json::array response_array;
    response_array.reserve(summary_values.size());
    for(const auto& summary_value : summary_values){
        response_array.push_back(make_problem_summary_object(summary_value));
    }
    return response_array;
}

boost::json::object json_util::make_problem_list_object(
    const std::vector<problem_dto::summary>& summary_values
){
    boost::json::object response_object;
    response_object["problem_count"] = static_cast<std::int64_t>(summary_values.size());
    response_object["problems"] = make_problem_summary_array(summary_values);
    return response_object;
}

boost::json::object json_util::make_problem_testcase_object(
    const problem_dto::testcase& testcase_value
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_value.id;
    response_object["testcase_order"] = testcase_value.order;
    response_object["testcase_input"] = testcase_value.input;
    response_object["testcase_output"] = testcase_value.output;
    return response_object;
}

boost::json::array json_util::make_problem_testcase_array(
    const std::vector<problem_dto::testcase>& testcase_values
){
    boost::json::array response_array;
    response_array.reserve(testcase_values.size());
    for(const auto& testcase_value : testcase_values){
        response_array.push_back(make_problem_testcase_object(testcase_value));
    }
    return response_array;
}

boost::json::object json_util::make_problem_testcase_list_object(
    const std::vector<problem_dto::testcase>& testcase_values
){
    boost::json::object response_object;
    response_object["testcase_count"] = static_cast<std::int64_t>(testcase_values.size());
    response_object["testcases"] = make_problem_testcase_array(testcase_values);
    return response_object;
}

boost::json::object json_util::make_problem_testcase_created_object(
    const problem_dto::testcase& testcase_value
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_value.id;
    response_object["testcase_order"] = testcase_value.order;
    return response_object;
}

boost::json::object json_util::make_problem_limits_object(
    const problem_content_dto::limits& limits_value
){
    boost::json::object limits_object;
    limits_object["memory_limit_mb"] = limits_value.memory_mb;
    limits_object["time_limit_ms"] = limits_value.time_ms;
    return limits_object;
}

boost::json::object json_util::make_problem_statement_object(
    const problem_content_dto::statement& statement_value
){
    boost::json::object statement_object;
    statement_object["description"] = statement_value.description;
    statement_object["input_format"] = statement_value.input_format;
    statement_object["output_format"] = statement_value.output_format;
    if(statement_value.note){
        statement_object["note"] = *statement_value.note;
    }
    return statement_object;
}

boost::json::object json_util::make_problem_sample_object(
    const problem_content_dto::sample& sample_value
){
    boost::json::object response_object;
    response_object["sample_id"] = sample_value.id;
    response_object["sample_order"] = sample_value.order;
    response_object["sample_input"] = sample_value.input;
    response_object["sample_output"] = sample_value.output;
    return response_object;
}

boost::json::array json_util::make_problem_sample_array(
    const std::vector<problem_content_dto::sample>& sample_values
){
    boost::json::array response_array;
    response_array.reserve(sample_values.size());
    for(const auto& sample_value : sample_values){
        response_array.push_back(make_problem_sample_object(sample_value));
    }
    return response_array;
}

boost::json::object json_util::make_problem_sample_list_object(
    const std::vector<problem_content_dto::sample>& sample_values
){
    boost::json::object response_object;
    response_object["sample_count"] = static_cast<std::int64_t>(sample_values.size());
    response_object["samples"] = make_problem_sample_array(sample_values);
    return response_object;
}

boost::json::object json_util::make_problem_sample_created_object(
    const problem_content_dto::sample& sample_value
){
    boost::json::object response_object;
    response_object["sample_id"] = sample_value.id;
    response_object["sample_order"] = sample_value.order;
    return response_object;
}

boost::json::array json_util::make_problem_samples_array(
    const std::vector<problem_content_dto::sample>& sample_values
){
    boost::json::array samples_array;
    samples_array.reserve(sample_values.size());
    for(const auto& sample_value : sample_values){
        boost::json::object sample_object;
        sample_object["sample_order"] = sample_value.order;
        sample_object["sample_input"] = sample_value.input;
        sample_object["sample_output"] = sample_value.output;
        samples_array.push_back(std::move(sample_object));
    }
    return samples_array;
}

boost::json::object json_util::make_problem_statistics_object(
    const problem_content_dto::statistics& statistics_value
){
    boost::json::object statistics_object;
    statistics_object["submission_count"] = statistics_value.submission_count;
    statistics_object["accepted_count"] = statistics_value.accepted_count;
    return statistics_object;
}

boost::json::object json_util::make_problem_detail_object(
    const problem_dto::reference& problem_reference_value,
    const problem_dto::title& title_value,
    const problem_dto::version& version_value,
    const problem_content_dto::limits& limits_value,
    const std::optional<problem_content_dto::statement>& statement_opt,
    const std::vector<problem_content_dto::sample>& sample_values,
    const problem_content_dto::statistics& statistics_value
){
    boost::json::object response_object;
    response_object["problem_id"] = problem_reference_value.problem_id;
    response_object["title"] = title_value.value;
    response_object["version"] = version_value.version;
    response_object["limits"] = make_problem_limits_object(limits_value);
    response_object["sample_count"] = static_cast<std::int64_t>(sample_values.size());
    response_object["samples"] = make_problem_samples_array(sample_values);
    response_object["statistics"] = make_problem_statistics_object(statistics_value);
    if(statement_opt){
        response_object["statement"] = make_problem_statement_object(*statement_opt);
    }
    else{
        response_object["statement"] = nullptr;
    }

    return response_object;
}
