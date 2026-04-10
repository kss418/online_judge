#pragma once

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace problem_json_serializer{
    boost::json::object make_created_object(
        const problem_dto::created& created_value
    );

    boost::json::object make_list_object(
        const std::vector<problem_dto::summary>& summary_values,
        std::int64_t total_problem_count
    );

    boost::json::object make_detail_object(
        const problem_dto::detail& detail_value
    );

    boost::json::object make_detail_object(
        const problem_dto::reference& problem_reference_value,
        const problem_dto::title& title_value,
        const problem_dto::version& version_value,
        const problem_content_dto::limits& limits_value,
        const std::optional<problem_content_dto::statement>& statement_opt,
        const std::vector<problem_content_dto::sample>& sample_values,
        const problem_content_dto::statistics& statistics_value,
        const std::optional<std::string>& user_problem_state_opt
    );

    boost::json::object make_testcase_object(
        const problem_dto::testcase& testcase_value
    );

    boost::json::object make_testcase_object(
        const problem_dto::testcase_mutation_result& testcase_value
    );

    boost::json::object make_testcase_summary_object(
        const problem_dto::testcase_summary& testcase_summary_value
    );

    boost::json::object make_testcase_list_object(
        const std::vector<problem_dto::testcase>& testcase_values
    );

    boost::json::object make_testcase_summary_list_object(
        const std::vector<problem_dto::testcase_summary>& testcase_summary_values
    );

    boost::json::object make_testcase_created_object(
        const problem_dto::testcase& testcase_value
    );

    boost::json::object make_testcase_created_object(
        const problem_dto::testcase_mutation_result& testcase_value
    );

    boost::json::object make_sample_object(
        const problem_content_dto::sample& sample_value
    );

    boost::json::object make_sample_object(
        const problem_dto::sample_mutation_result& sample_value
    );

    boost::json::object make_sample_list_object(
        const std::vector<problem_content_dto::sample>& sample_values
    );

    boost::json::object make_sample_created_object(
        const problem_content_dto::sample& sample_value
    );

    boost::json::object make_sample_created_object(
        const problem_dto::sample_mutation_result& sample_value
    );

    boost::json::object make_message_object(
        std::string_view message,
        const problem_dto::mutation_result& mutation_value
    );

    boost::json::object make_testcase_count_message_object(
        std::string_view message,
        const problem_dto::testcase_count_mutation_result& testcase_count_value
    );
}
