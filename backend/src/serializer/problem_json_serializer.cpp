#include "serializer/problem_json_serializer.hpp"

#include "serializer/common_json_serializer.hpp"

#include <utility>

namespace{
    void append_problem_version_fields(
        boost::json::object& response_object,
        const problem_dto::mutation_result& mutation_value
    ){
        response_object["problem_id"] = mutation_value.problem_id;
        response_object["version"] = mutation_value.version;
    }

    boost::json::object make_summary_object(
        const problem_dto::summary& summary_value
    ){
        boost::json::object response_object;
        response_object["problem_id"] = summary_value.problem_id;
        response_object["title"] = summary_value.title;
        response_object["version"] = summary_value.version;
        response_object["time_limit_ms"] = summary_value.time_limit_ms;
        response_object["memory_limit_mb"] = summary_value.memory_limit_mb;
        response_object["submission_count"] = summary_value.submission_count;
        response_object["accepted_count"] = summary_value.accepted_count;
        if(summary_value.user_problem_state_opt){
            response_object["user_problem_state"] = *summary_value.user_problem_state_opt;
        }
        else{
            response_object["user_problem_state"] = nullptr;
        }
        return response_object;
    }

    boost::json::array make_summary_array(
        const std::vector<problem_dto::summary>& summary_values
    ){
        boost::json::array response_array;
        response_array.reserve(summary_values.size());
        for(const auto& summary_value : summary_values){
            response_array.push_back(make_summary_object(summary_value));
        }
        return response_array;
    }

    boost::json::array make_testcase_array(
        const std::vector<problem_dto::testcase>& testcase_values
    ){
        boost::json::array response_array;
        response_array.reserve(testcase_values.size());
        for(const auto& testcase_value : testcase_values){
            response_array.push_back(problem_json_serializer::make_testcase_object(testcase_value));
        }
        return response_array;
    }

    boost::json::array make_testcase_summary_array(
        const std::vector<problem_dto::testcase_summary>& testcase_summary_values
    ){
        boost::json::array response_array;
        response_array.reserve(testcase_summary_values.size());
        for(const auto& testcase_summary_value : testcase_summary_values){
            response_array.push_back(
                problem_json_serializer::make_testcase_summary_object(testcase_summary_value)
            );
        }
        return response_array;
    }

    boost::json::object make_limits_object(
        const problem_content_dto::limits& limits_value
    ){
        boost::json::object limits_object;
        limits_object["memory_limit_mb"] = limits_value.memory_mb;
        limits_object["time_limit_ms"] = limits_value.time_ms;
        return limits_object;
    }

    boost::json::object make_statement_object(
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

    boost::json::array make_sample_array(
        const std::vector<problem_content_dto::sample>& sample_values
    ){
        boost::json::array response_array;
        response_array.reserve(sample_values.size());
        for(const auto& sample_value : sample_values){
            response_array.push_back(problem_json_serializer::make_sample_object(sample_value));
        }
        return response_array;
    }

    boost::json::array make_samples_array(
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

    boost::json::object make_statistics_object(
        const problem_content_dto::statistics& statistics_value
    ){
        boost::json::object statistics_object;
        statistics_object["submission_count"] = statistics_value.submission_count;
        statistics_object["accepted_count"] = statistics_value.accepted_count;
        return statistics_object;
    }
}

boost::json::object problem_json_serializer::make_created_object(
    const problem_dto::created& created_value
){
    boost::json::object response_object;
    response_object["problem_id"] = created_value.problem_id;
    return response_object;
}

boost::json::object problem_json_serializer::make_list_object(
    const std::vector<problem_dto::summary>& summary_values,
    std::int64_t total_problem_count
){
    boost::json::object response_object;
    response_object["problem_count"] = static_cast<std::int64_t>(summary_values.size());
    response_object["total_problem_count"] = total_problem_count;
    response_object["problems"] = make_summary_array(summary_values);
    return response_object;
}

boost::json::object problem_json_serializer::make_detail_object(
    const problem_dto::detail& detail_value
){
    return make_detail_object(
        detail_value.problem_reference_value,
        detail_value.title_value,
        detail_value.version_value,
        detail_value.limits_value,
        detail_value.statement_opt,
        detail_value.sample_values,
        detail_value.statistics_value,
        detail_value.user_problem_state_opt
    );
}

boost::json::object problem_json_serializer::make_detail_object(
    const problem_dto::reference& problem_reference_value,
    const problem_dto::title& title_value,
    const problem_dto::version& version_value,
    const problem_content_dto::limits& limits_value,
    const std::optional<problem_content_dto::statement>& statement_opt,
    const std::vector<problem_content_dto::sample>& sample_values,
    const problem_content_dto::statistics& statistics_value,
    const std::optional<std::string>& user_problem_state_opt
){
    boost::json::object response_object;
    response_object["problem_id"] = problem_reference_value.problem_id;
    response_object["title"] = title_value.value;
    response_object["version"] = version_value.version;
    response_object["limits"] = make_limits_object(limits_value);
    response_object["sample_count"] = static_cast<std::int64_t>(sample_values.size());
    response_object["samples"] = make_samples_array(sample_values);
    response_object["statistics"] = make_statistics_object(statistics_value);
    if(statement_opt){
        response_object["statement"] = make_statement_object(*statement_opt);
    }
    else{
        response_object["statement"] = nullptr;
    }
    if(user_problem_state_opt){
        response_object["user_problem_state"] = *user_problem_state_opt;
    }
    else{
        response_object["user_problem_state"] = nullptr;
    }

    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_object(
    const problem_dto::testcase& testcase_value
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_value.id;
    response_object["testcase_order"] = testcase_value.order;
    response_object["testcase_input"] = testcase_value.input;
    response_object["testcase_output"] = testcase_value.output;
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_object(
    const problem_dto::testcase_mutation_result& testcase_value
){
    auto response_object = make_testcase_object(testcase_value.testcase_value);
    append_problem_version_fields(
        response_object,
        testcase_value.problem_value
    );
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_summary_object(
    const problem_dto::testcase_summary& testcase_summary_value
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_summary_value.id;
    response_object["testcase_order"] = testcase_summary_value.order;
    response_object["input_char_count"] = testcase_summary_value.input_char_count;
    response_object["input_line_count"] = testcase_summary_value.input_line_count;
    response_object["output_char_count"] = testcase_summary_value.output_char_count;
    response_object["output_line_count"] = testcase_summary_value.output_line_count;
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_list_object(
    const std::vector<problem_dto::testcase>& testcase_values
){
    boost::json::object response_object;
    response_object["testcase_count"] = static_cast<std::int64_t>(testcase_values.size());
    response_object["testcases"] = make_testcase_array(testcase_values);
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_summary_list_object(
    const std::vector<problem_dto::testcase_summary>& testcase_summary_values
){
    boost::json::object response_object;
    response_object["testcase_count"] = static_cast<std::int64_t>(testcase_summary_values.size());
    response_object["testcases"] = make_testcase_summary_array(testcase_summary_values);
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_created_object(
    const problem_dto::testcase& testcase_value
){
    boost::json::object response_object;
    response_object["testcase_id"] = testcase_value.id;
    response_object["testcase_order"] = testcase_value.order;
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_created_object(
    const problem_dto::testcase_mutation_result& testcase_value
){
    auto response_object = make_testcase_created_object(testcase_value.testcase_value);
    append_problem_version_fields(
        response_object,
        testcase_value.problem_value
    );
    return response_object;
}

boost::json::object problem_json_serializer::make_sample_object(
    const problem_content_dto::sample& sample_value
){
    boost::json::object response_object;
    response_object["sample_id"] = sample_value.id;
    response_object["sample_order"] = sample_value.order;
    response_object["sample_input"] = sample_value.input;
    response_object["sample_output"] = sample_value.output;
    return response_object;
}

boost::json::object problem_json_serializer::make_sample_object(
    const problem_dto::sample_mutation_result& sample_value
){
    auto response_object = make_sample_object(sample_value.sample_value);
    append_problem_version_fields(
        response_object,
        sample_value.problem_value
    );
    return response_object;
}

boost::json::object problem_json_serializer::make_sample_list_object(
    const std::vector<problem_content_dto::sample>& sample_values
){
    boost::json::object response_object;
    response_object["sample_count"] = static_cast<std::int64_t>(sample_values.size());
    response_object["samples"] = make_sample_array(sample_values);
    return response_object;
}

boost::json::object problem_json_serializer::make_sample_created_object(
    const problem_content_dto::sample& sample_value
){
    boost::json::object response_object;
    response_object["sample_id"] = sample_value.id;
    response_object["sample_order"] = sample_value.order;
    return response_object;
}

boost::json::object problem_json_serializer::make_sample_created_object(
    const problem_dto::sample_mutation_result& sample_value
){
    auto response_object = make_sample_created_object(sample_value.sample_value);
    append_problem_version_fields(
        response_object,
        sample_value.problem_value
    );
    return response_object;
}

boost::json::object problem_json_serializer::make_message_object(
    std::string_view message,
    const problem_dto::mutation_result& mutation_value
){
    auto response_object = common_json_serializer::make_message_object(message);
    append_problem_version_fields(response_object, mutation_value);
    return response_object;
}

boost::json::object problem_json_serializer::make_testcase_count_message_object(
    std::string_view message,
    const problem_dto::testcase_count_mutation_result& testcase_count_value
){
    auto response_object = make_message_object(
        message,
        testcase_count_value.problem_value
    );
    response_object["testcase_count"] = testcase_count_value.testcase_count_value.testcase_count;
    return response_object;
}
