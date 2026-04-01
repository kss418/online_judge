#pragma once

#include "dto/dto_validation_error.hpp"
#include "dto/problem_dto.hpp"
#include "dto/submission_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/request_parser.hpp"

#include <expected>
#include <functional>
#include <string_view>
#include <utility>

namespace request_dto{
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
}
