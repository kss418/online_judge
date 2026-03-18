#pragma once

#include "dto/problem_dto.hpp"

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

    boost::json::object make_auth_session_object(
        std::int64_t user_id,
        bool is_admin,
        std::string_view token
    );

    boost::json::object make_submission_created_object(
        std::int64_t submission_id,
        std::string_view status_value
    );

    boost::json::object make_problem_created_object(std::int64_t problem_id);
    boost::json::object make_problem_limits_object(const limits& limits_value);
    boost::json::object make_problem_statement_object(const problem_statement& statement_value);
    boost::json::array make_problem_samples_array(const std::vector<sample>& sample_values);
    boost::json::object make_problem_statistics_object(
        const problem_statistics& statistics_value
    );
    boost::json::object make_problem_detail_object(
        std::int64_t problem_id,
        std::int32_t version,
        const limits& limits_value,
        const std::optional<problem_statement>& statement_opt,
        const std::vector<sample>& sample_values,
        const problem_statistics& statistics_value
    );
}
