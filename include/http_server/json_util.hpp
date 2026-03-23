#pragma once

#include "dto/auth_dto.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "dto/submission_dto.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace json_util{
    boost::json::object make_error_object(
        std::string_view code,
        std::string_view message,
        std::optional<std::string> field_opt = std::nullopt
    );
    boost::json::object make_message_object(std::string_view message);

    boost::json::object make_auth_session_object(const auth_dto::session& session_value);

    boost::json::object make_submission_created_object(
        const submission_dto::created& created_value
    );
    boost::json::object make_submission_summary_object(
        const submission_dto::summary& summary_value
    );
    boost::json::object make_submission_detail_object(
        const submission_dto::detail& detail_value
    );
    boost::json::array make_submission_summary_array(
        const std::vector<submission_dto::summary>& summary_values
    );
    boost::json::object make_submission_list_object(
        const std::vector<submission_dto::summary>& summary_values
    );

    boost::json::object make_problem_created_object(const problem_dto::created& created_value);
    boost::json::object make_problem_summary_object(const problem_dto::summary& summary_value);
    boost::json::array make_problem_summary_array(
        const std::vector<problem_dto::summary>& summary_values
    );
    boost::json::object make_problem_list_object(
        const std::vector<problem_dto::summary>& summary_values
    );
    boost::json::object make_problem_testcase_object(
        const problem_dto::testcase& testcase_value
    );
    boost::json::array make_problem_testcase_array(
        const std::vector<problem_dto::testcase>& testcase_values
    );
    boost::json::object make_problem_testcase_list_object(
        const std::vector<problem_dto::testcase>& testcase_values
    );
    boost::json::object make_problem_testcase_created_object(
        const problem_dto::testcase& testcase_value
    );
    boost::json::object make_problem_limits_object(
        const problem_content_dto::limits& limits_value
    );
    boost::json::object make_problem_statement_object(
        const problem_content_dto::statement& statement_value
    );
    boost::json::object make_problem_sample_object(
        const problem_content_dto::sample& sample_value
    );
    boost::json::array make_problem_sample_array(
        const std::vector<problem_content_dto::sample>& sample_values
    );
    boost::json::object make_problem_sample_list_object(
        const std::vector<problem_content_dto::sample>& sample_values
    );
    boost::json::object make_problem_sample_created_object(
        const problem_content_dto::sample& sample_value
    );
    boost::json::array make_problem_samples_array(
        const std::vector<problem_content_dto::sample>& sample_values
    );
    boost::json::object make_problem_statistics_object(
        const problem_content_dto::statistics& statistics_value
    );
    boost::json::object make_problem_detail_object(
        const problem_dto::reference& problem_reference_value,
        const problem_dto::title& title_value,
        const problem_dto::version& version_value,
        const problem_content_dto::limits& limits_value,
        const std::optional<problem_content_dto::statement>& statement_opt,
        const std::vector<problem_content_dto::sample>& sample_values,
        const problem_content_dto::statistics& statistics_value
    );
}
