#pragma once

#include "common/language_util.hpp"
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
    boost::json::object make_user_permission_object(
        std::int64_t user_id,
        std::int32_t permission_level
    );
    boost::json::object make_user_me_object(const auth_dto::identity& auth_identity_value);
    boost::json::object make_user_summary_object(
        const auth_dto::user_summary& user_summary_value
    );
    boost::json::array make_user_summary_array(
        const auth_dto::user_summary_list& user_summary_values
    );
    boost::json::object make_user_list_object(
        const auth_dto::user_summary_list& user_summary_values
    );
    boost::json::object make_supported_language_object(
        const language_util::supported_language& supported_language_value
    );
    boost::json::array make_supported_language_array(
        std::span<const language_util::supported_language> supported_language_values
    );
    boost::json::object make_supported_language_list_object(
        std::span<const language_util::supported_language> supported_language_values
    );

    boost::json::object make_auth_session_object(const auth_dto::session& session_value);

    boost::json::object make_submission_created_object(
        const submission_dto::created& created_value
    );
    boost::json::object make_submission_history_object(
        const submission_dto::history& history_value
    );
    boost::json::array make_submission_history_array(
        const submission_dto::history_list& history_values
    );
    boost::json::object make_submission_history_list_object(
        std::int64_t submission_id,
        const submission_dto::history_list& history_values
    );
    boost::json::object make_submission_source_object(
        const submission_dto::source_detail& source_detail_value
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
        const std::vector<submission_dto::summary>& summary_values,
        std::int64_t total_submission_count
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
        const problem_content_dto::statistics& statistics_value,
        const std::optional<std::string>& user_problem_state_opt
    );
}
