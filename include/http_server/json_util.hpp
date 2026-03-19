#pragma once

#include "dto/auth_dto.hpp"
#include "dto/problem_dto.hpp"
#include "dto/submission_dto.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace json_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type create_json_response(
        const request_type& request,
        boost::beast::http::status status,
        const boost::json::value& response_value
    );

    boost::json::object make_auth_session_object(const auth_dto::session& session_value);

    boost::json::object make_submission_created_object(
        const submission_dto::created& created_value
    );
    boost::json::object make_submission_summary_object(
        const submission_dto::summary& summary_value
    );
    boost::json::array make_submission_summary_array(
        const std::vector<submission_dto::summary>& summary_values
    );
    boost::json::object make_submission_list_object(
        const std::vector<submission_dto::summary>& summary_values
    );

    boost::json::object make_problem_created_object(const problem_dto::created& created_value);
    boost::json::object make_problem_testcase_created_object(
        const problem_dto::testcase& testcase_value
    );
    boost::json::object make_problem_limits_object(const problem_dto::limits& limits_value);
    boost::json::object make_problem_statement_object(
        const problem_dto::statement& statement_value
    );
    boost::json::array make_problem_samples_array(
        const std::vector<problem_dto::sample>& sample_values
    );
    boost::json::object make_problem_statistics_object(
        const problem_dto::statistics& statistics_value
    );
    boost::json::object make_problem_detail_object(
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value,
        const problem_dto::limits& limits_value,
        const std::optional<problem_dto::statement>& statement_opt,
        const std::vector<problem_dto::sample>& sample_values,
        const problem_dto::statistics& statistics_value
    );
}
