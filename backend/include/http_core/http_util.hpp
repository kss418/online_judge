#pragma once

#include "http_core/auth_guard.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/request_parser.hpp"

#include "common/db_connection.hpp"
#include "dto/dto_validation_error.hpp"
#include "dto/problem_dto.hpp"
#include "dto/submission_dto.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace http_util{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    template <typename dto_type, typename factory_type, typename... arg_types>
    std::expected<dto_type, response_type> parse_json_dto_or_400(
        const request_type& request,
        factory_type&& factory,
        arg_types&&... args
    ){
        const auto request_object_opt = request_parser::parse_json_object(request);
        if(!request_object_opt){
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_json",
                "invalid json"
            ));
        }

        auto dto_exp = std::invoke(
            std::forward<factory_type>(factory),
            *request_object_opt,
            std::forward<arg_types>(args)...
        );
        if(!dto_exp){
            const auto& validation_error = dto_exp.error();
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                validation_error.code,
                validation_error.message,
                validation_error.field_opt
            ));
        }

        return std::move(*dto_exp);
    }
    
    template <typename dto_type, typename factory_type, typename... arg_types>
    std::expected<dto_type, response_type> parse_query_dto_or_400(
        const request_type& request,
        factory_type&& factory,
        arg_types&&... args
    ){
        const std::string_view target{
            request.target().data(),
            request.target().size()
        };
        const auto query_opt = request_parser::get_target_query(target);
        const auto query_params_opt = request_parser::parse_query_params(query_opt.value_or(""));
        if(!query_params_opt){
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_query_string",
                "invalid query string"
            ));
        }

        auto dto_exp = std::invoke(
            std::forward<factory_type>(factory),
            *query_params_opt,
            std::forward<arg_types>(args)...
        );
        if(!dto_exp){
            const auto& validation_error = dto_exp.error();
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                validation_error.code,
                validation_error.message,
                validation_error.field_opt
            ));
        }

        return std::move(*dto_exp);
    }
    std::expected<submission_dto::list_filter, response_type>
    parse_submission_list_filter_or_400(
        const request_type& request
    );
    std::expected<problem_dto::list_filter, response_type>
    parse_problem_list_filter_or_400(
        const request_type& request
    );
    std::expected<void, response_type> require_existing_problem_or_response(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value
    );
    template <typename callback_type>
    response_type with_existing_problem(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ){
        const auto require_problem_exp = require_existing_problem_or_response(
            request,
            db_connection,
            problem_reference_value
        );
        if(!require_problem_exp){
            return std::move(require_problem_exp.error());
        }

        return std::invoke(std::forward<callback_type>(callback));
    }
    bool is_owner_or_admin(
        const auth_dto::identity& auth_identity_value,
        std::int64_t owner_user_id
    );
    template <typename callback_type>
    response_type with_existing_problem_admin(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ){
        auto&& callback_ref = callback;
        return auth_guard::with_admin_auth_bearer(
            request,
            db_connection,
            [&](const auth_dto::identity& auth_identity) -> response_type {
                const auto require_problem_exp = require_existing_problem_or_response(
                    request,
                    db_connection,
                    problem_reference_value
                );
                if(!require_problem_exp){
                    return std::move(require_problem_exp.error());
                }

                return std::invoke(callback_ref, auth_identity);
            }
        );
    }
}
