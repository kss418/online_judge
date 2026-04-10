#pragma once

#include "http_core/http_response_util.hpp"

#include <boost/beast/http/status.hpp>

#include <expected>
#include <string_view>
#include <utility>

namespace http_adapter{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    namespace detail{
        template <typename Error, typename ErrorHandler, typename SuccessHandler>
        response_type create_or_error(
            const request_type& request,
            std::expected<void, Error> value_exp,
            ErrorHandler&& error_handler,
            SuccessHandler&& success_handler
        ){
            if(!value_exp){
                return std::forward<ErrorHandler>(error_handler)(
                    request,
                    value_exp.error()
                );
            }

            return std::forward<SuccessHandler>(success_handler)();
        }

        template <typename T, typename Error, typename ErrorHandler, typename SuccessHandler>
        response_type create_or_error(
            const request_type& request,
            std::expected<T, Error> value_exp,
            ErrorHandler&& error_handler,
            SuccessHandler&& success_handler
        ){
            if(!value_exp){
                return std::forward<ErrorHandler>(error_handler)(
                    request,
                    value_exp.error()
                );
            }

            return std::forward<SuccessHandler>(success_handler)(std::move(*value_exp));
        }
    }

    inline response_type error(
        const request_type& request,
        const http_error& error_value
    ){
        return http_response_util::create_error(request, error_value);
    }

    template <typename JsonValue>
    response_type json(
        const request_type& request,
        JsonValue&& response_value,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return http_response_util::create_json(
            request,
            success_status,
            std::forward<JsonValue>(response_value)
        );
    }

    inline response_type message(
        const request_type& request,
        std::string_view message_value,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return http_response_util::create_message(
            request,
            success_status,
            message_value
        );
    }

    template <typename Error, typename HttpErrorMapper>
    response_type error_or_4xx_or_500(
        const request_type& request,
        const Error& error_value,
        HttpErrorMapper&& http_error_mapper
    ){
        if(const auto mapped_error_opt =
            std::forward<HttpErrorMapper>(http_error_mapper)(error_value)){
            return http_response_util::create_error(
                request,
                *mapped_error_opt
            );
        }

        return http_response_util::create_4xx_or_500(request, error_value);
    }

    template <typename Error, typename HttpErrorMapper, typename ServiceErrorSelector>
    response_type error_or_4xx_or_500(
        const request_type& request,
        const Error& error_value,
        HttpErrorMapper&& http_error_mapper,
        ServiceErrorSelector&& service_error_selector
    ){
        if(const auto mapped_error_opt =
            std::forward<HttpErrorMapper>(http_error_mapper)(error_value)){
            return http_response_util::create_error(
                request,
                *mapped_error_opt
            );
        }

        return http_response_util::create_4xx_or_500(
            request,
            std::forward<ServiceErrorSelector>(service_error_selector)(error_value)
        );
    }

    template <typename T, typename Error, typename ErrorHandler, typename SuccessHandler>
    response_type response(
        const request_type& request,
        std::expected<T, Error> value_exp,
        ErrorHandler&& error_handler,
        SuccessHandler&& success_handler
    ){
        return detail::create_or_error(
            request,
            std::move(value_exp),
            std::forward<ErrorHandler>(error_handler),
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename Error, typename SuccessHandler>
    response_type response(
        const request_type& request,
        std::expected<T, Error> value_exp,
        SuccessHandler&& success_handler
    ){
        return response(
            request,
            std::move(value_exp),
            [](const request_type& request_value, const Error& error_value) {
                return http_response_util::create_4xx_or_500(
                    request_value,
                    error_value
                );
            },
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename Error, typename ErrorHandler, typename Serializer>
    response_type json(
        const request_type& request,
        std::expected<T, Error> value_exp,
        ErrorHandler&& error_handler,
        Serializer&& serializer,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            std::move(value_exp),
            std::forward<ErrorHandler>(error_handler),
            [&](auto&&... args) -> response_type {
                return http_response_util::create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)(
                        std::forward<decltype(args)>(args)...
                    )
                );
            }
        );
    }

    template <typename T, typename Error, typename Serializer>
    response_type json(
        const request_type& request,
        std::expected<T, Error> value_exp,
        Serializer&& serializer,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return json(
            request,
            std::move(value_exp),
            [](const request_type& request_value, const Error& error_value) {
                return http_response_util::create_4xx_or_500(
                    request_value,
                    error_value
                );
            },
            std::forward<Serializer>(serializer),
            success_status
        );
    }

    template <typename T, typename Error, typename ErrorHandler>
    response_type message(
        const request_type& request,
        std::expected<T, Error> value_exp,
        ErrorHandler&& error_handler,
        std::string_view message_value,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            std::move(value_exp),
            std::forward<ErrorHandler>(error_handler),
            [&]() -> response_type {
                return http_response_util::create_message(
                    request,
                    success_status,
                    message_value
                );
            }
        );
    }

    template <typename T, typename Error>
    response_type message(
        const request_type& request,
        std::expected<T, Error> value_exp,
        std::string_view message_value,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return message(
            request,
            std::move(value_exp),
            [](const request_type& request_value, const Error& error_value) {
                return http_response_util::create_4xx_or_500(
                    request_value,
                    error_value
                );
            },
            message_value,
            success_status
        );
    }
}
