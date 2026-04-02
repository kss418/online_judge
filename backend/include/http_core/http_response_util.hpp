#pragma once

#include "error/http_error.hpp"
#include "error/service_error.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace http_response_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    namespace detail{
        template <typename Error, typename ErrorHandler, typename SuccessHandler>
        response_type create_or_error(
            const request_type& request,
            std::expected<void, Error> value_exp,
            ErrorHandler&& error_handler,
            SuccessHandler&& success_handler
        ){
            if(!value_exp){
                return std::forward<ErrorHandler>(error_handler)(request, value_exp.error());
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
                return std::forward<ErrorHandler>(error_handler)(request, value_exp.error());
            }

            return std::forward<SuccessHandler>(success_handler)(std::move(*value_exp));
        }
    }

    response_type create_json(
        const request_type& request,
        boost::beast::http::status status,
        const boost::json::value& response_value
    );
    response_type create_message(
        const request_type& request,
        boost::beast::http::status status,
        std::string_view message
    );
    response_type create_error(
        const request_type& request,
        const http_error& error
    );
    response_type create_4xx_or_500(
        const request_type& request,
        const service_error& code
    );
    response_type create_internal_server_error(
        const request_type& request,
        std::string_view context,
        std::string_view detail = {}
    );
    response_type create_service_unavailable(
        const request_type& request,
        std::string_view message = {}
    );
    response_type create_method_not_allowed(
        const request_type& request
    );
    response_type create_not_found(
        const request_type& request
    );

    template <typename T, typename Error, typename ErrorHandler, typename SuccessHandler>
    response_type create_response_or_error(
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
    response_type create_response_or_4xx_or_500(
        const request_type& request,
        std::expected<T, Error> value_exp,
        SuccessHandler&& success_handler
    ){
        auto error_handler = [](
            const request_type& request_value,
            const Error& error_value
        ) -> response_type {
            return create_4xx_or_500(request_value, error_value);
        };
        return create_response_or_error(
            request,
            std::move(value_exp),
            error_handler,
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename Error, typename Serializer>
    response_type create_json_or_4xx_or_500(
        const request_type& request,
        std::expected<T, Error> value_exp,
        Serializer&& serializer,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        auto error_handler = [](
            const request_type& request_value,
            const Error& error_value
        ) -> response_type {
            return create_4xx_or_500(request_value, error_value);
        };
        return detail::create_or_error(
            request,
            std::move(value_exp),
            error_handler,
            [&](auto&&... args) -> response_type {
                return create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)(
                        std::forward<decltype(args)>(args)...
                    )
                );
            }
        );
    }

    template <typename T, typename Error>
    response_type create_message_or_4xx_or_500(
        const request_type& request,
        std::expected<T, Error> value_exp,
        std::string_view message,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        auto error_handler = [](
            const request_type& request_value,
            const Error& error_value
        ) -> response_type {
            return create_4xx_or_500(request_value, error_value);
        };
        return detail::create_or_error(
            request,
            std::move(value_exp),
            error_handler,
            [&](auto&&...) -> response_type {
                return create_message(request, success_status, message);
            }
        );
    }

}
