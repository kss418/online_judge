#pragma once

#include "common/error_code.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace http_response_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    namespace detail{
        template <typename ErrorHandler, typename SuccessHandler>
        response_type create_or_error(
            const request_type& request,
            std::string_view action,
            std::expected<void, error_code> value_exp,
            ErrorHandler&& error_handler,
            SuccessHandler&& success_handler
        ){
            if(!value_exp){
                return std::forward<ErrorHandler>(error_handler)(
                    request,
                    action,
                    value_exp.error()
                );
            }

            return std::forward<SuccessHandler>(success_handler)();
        }

        template <typename T, typename ErrorHandler, typename SuccessHandler>
        response_type create_or_error(
            const request_type& request,
            std::string_view action,
            std::expected<T, error_code> value_exp,
            ErrorHandler&& error_handler,
            SuccessHandler&& success_handler
        ){
            if(!value_exp){
                return std::forward<ErrorHandler>(error_handler)(
                    request,
                    action,
                    value_exp.error()
                );
            }

            return std::forward<SuccessHandler>(success_handler)(std::move(*value_exp));
        }
    }

    response_type create_text(
        const request_type& request,
        boost::beast::http::status status,
        std::string body
    );
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
        boost::beast::http::status status,
        std::string_view code,
        std::string_view message,
        std::optional<std::string> field_opt = std::nullopt
    );
    response_type create_bearer_error(
        const request_type& request,
        std::string_view code,
        std::string_view message
    );
    response_type create_4xx_or_500(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_404_or_500(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_bearer_unauthorized(
        const request_type& request,
        std::string body
    );
    response_type create_method_not_allowed(
        const request_type& request
    );
    response_type create_not_found(
        const request_type& request
    );
    response_type create_user_not_found(
        const request_type& request
    );

    template <typename T, typename ErrorHandler, typename SuccessHandler>
    response_type create_response_or_error(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        ErrorHandler&& error_handler,
        SuccessHandler&& success_handler
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            std::forward<ErrorHandler>(error_handler),
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename SuccessHandler>
    response_type create_response_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        SuccessHandler&& success_handler
    ){
        return create_response_or_error(
            request,
            action,
            std::move(value_exp),
            create_4xx_or_500,
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename SuccessHandler>
    response_type create_response_or_404_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        SuccessHandler&& success_handler
    ){
        return create_response_or_error(
            request,
            action,
            std::move(value_exp),
            create_404_or_500,
            std::forward<SuccessHandler>(success_handler)
        );
    }

    template <typename T, typename Serializer>
    response_type create_json_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        Serializer&& serializer,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_4xx_or_500,
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

    template <typename T>
    response_type create_message_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        std::string_view message,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_4xx_or_500,
            [&](auto&&...) -> response_type {
                return create_message(request, success_status, message);
            }
        );
    }

    template <typename T, typename Serializer, typename NulloptHandler>
    response_type create_json_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<std::optional<T>, error_code> value_opt_exp,
        Serializer&& serializer,
        NulloptHandler&& nullopt_handler,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_opt_exp),
            create_4xx_or_500,
            [&](std::optional<T> value_opt) -> response_type {
                if(!value_opt.has_value()){
                    return std::forward<NulloptHandler>(nullopt_handler)();
                }

                return create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)(std::move(*value_opt))
                );
            }
        );
    }

    template <typename FalseHandler>
    response_type create_message_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<bool, error_code> value_exp,
        std::string_view message,
        FalseHandler&& false_handler,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_4xx_or_500,
            [&](bool value) -> response_type {
                if(!value){
                    return std::forward<FalseHandler>(false_handler)();
                }

                return create_message(request, success_status, message);
            }
        );
    }

    template <typename Serializer, typename FalseHandler>
    response_type create_json_or_4xx_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<bool, error_code> value_exp,
        Serializer&& serializer,
        FalseHandler&& false_handler,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_4xx_or_500,
            [&](bool value) -> response_type {
                if(!value){
                    return std::forward<FalseHandler>(false_handler)();
                }

                return create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)()
                );
            }
        );
    }

    template <typename T, typename Serializer>
    response_type create_json_or_404_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<T, error_code> value_exp,
        Serializer&& serializer,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_404_or_500,
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

    template <typename T, typename Serializer, typename NulloptHandler>
    response_type create_json_or_404_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<std::optional<T>, error_code> value_opt_exp,
        Serializer&& serializer,
        NulloptHandler&& nullopt_handler,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_opt_exp),
            create_404_or_500,
            [&](std::optional<T> value_opt) -> response_type {
                if(!value_opt.has_value()){
                    return std::forward<NulloptHandler>(nullopt_handler)();
                }

                return create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)(std::move(*value_opt))
                );
            }
        );
    }

    template <typename Serializer, typename FalseHandler>
    response_type create_json_or_404_or_500(
        const request_type& request,
        std::string_view action,
        std::expected<bool, error_code> value_exp,
        Serializer&& serializer,
        FalseHandler&& false_handler,
        boost::beast::http::status success_status = boost::beast::http::status::ok
    ){
        return detail::create_or_error(
            request,
            action,
            std::move(value_exp),
            create_404_or_500,
            [&](bool value) -> response_type {
                if(!value){
                    return std::forward<FalseHandler>(false_handler)();
                }

                return create_json(
                    request,
                    success_status,
                    std::forward<Serializer>(serializer)()
                );
            }
        );
    }
}
