#include "http_core/request_observer.hpp"

#include "common/logger.hpp"
#include "http_core/request_context.hpp"

#include <chrono>
#include <optional>
#include <string_view>

void logging_request_observer::on_request_complete(
    const request_context& context,
    const http_response_util::response_type& response,
    duration_type duration
) noexcept{
    try{
        const auto method = std::string_view{
            context.request.method_string().data(),
            context.request.method_string().size()
        };
        const auto target = std::string_view{
            context.request.target().data(),
            context.request.target().size()
        };
        const auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            duration
        ).count();
        const auto auth_user_id_opt = context.auth_identity_opt.has_value()
            ? std::optional<std::int64_t>{context.auth_identity_opt->user_id}
            : std::nullopt;

        logger::clog()
            .log("http_request_completed")
            .field("request_id", context.request_id)
            .field("method", method)
            .field("target", target)
            .field("status", response.result_int())
            .field("duration_us", duration_us)
            .field("response_bytes", response.body().size())
            .field("keep_alive", response.keep_alive() ? 1 : 0)
            .field("has_db_connection", context.has_db_connection() ? 1 : 0)
            .optional_field("user_id", auth_user_id_opt);
    }
    catch(...){
    }
}
