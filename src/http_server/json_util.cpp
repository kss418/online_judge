#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

json_util::response_type json_util::create_json_response(
    const request_type& request,
    boost::beast::http::status status,
    const boost::json::value& response_value
){
    auto response = http_util::create_text_response(
        request,
        status,
        boost::json::serialize(response_value) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

boost::json::object json_util::make_auth_session_object(const auth_dto::session& session_value){
    boost::json::object response_object;
    response_object["user_id"] = session_value.user_id;
    response_object["is_admin"] = session_value.is_admin;
    response_object["token"] = session_value.token;
    return response_object;
}

boost::json::object json_util::make_submission_created_object(
    std::int64_t submission_id,
    std::string_view status_value
){
    boost::json::object response_object;
    response_object["submission_id"] = submission_id;
    response_object["status"] = status_value;
    return response_object;
}

boost::json::object json_util::make_problem_created_object(std::int64_t problem_id){
    boost::json::object response_object;
    response_object["problem_id"] = problem_id;
    return response_object;
}

boost::json::object json_util::make_problem_testcase_created_object(
    std::int64_t testcase_id,
    std::int32_t testcase_order
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_id;
    response_object["testcase_order"] = testcase_order;
    return response_object;
}

boost::json::object json_util::make_problem_limits_object(
    const problem_dto::limits& limits_value
){
    boost::json::object limits_object;
    limits_object["memory_limit_mb"] = limits_value.memory_mb;
    limits_object["time_limit_ms"] = limits_value.time_ms;
    return limits_object;
}

boost::json::object json_util::make_problem_statement_object(
    const problem_dto::statement& statement_value
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

boost::json::array json_util::make_problem_samples_array(
    const std::vector<problem_dto::sample>& sample_values
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
    const problem_dto::statistics& statistics_value
){
    boost::json::object statistics_object;
    statistics_object["submission_count"] = statistics_value.submission_count;
    statistics_object["accepted_count"] = statistics_value.accepted_count;
    return statistics_object;
}

boost::json::object json_util::make_problem_detail_object(
    std::int64_t problem_id,
    std::int32_t version,
    const problem_dto::limits& limits_value,
    const std::optional<problem_dto::statement>& statement_opt,
    const std::vector<problem_dto::sample>& sample_values,
    const problem_dto::statistics& statistics_value
){
    boost::json::object response_object;
    response_object["problem_id"] = problem_id;
    response_object["version"] = version;
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
