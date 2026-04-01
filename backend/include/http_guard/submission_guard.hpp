#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "dto/submission_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_guard/guard_runner.hpp"

#include <cstdint>
#include <expected>

namespace submission_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<submission_dto::history_list, response_type> require_history(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::detail, response_type> require_detail(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::create_request, response_type> require_create_request(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t problem_id
    );
    std::expected<submission_dto::source_detail, response_type> require_source_detail(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, response_type> require_readable_source(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<void, response_type> require_source_access(
        const request_type& request,
        const auth_dto::identity& auth_identity_value,
        const submission_dto::source_detail& source_detail_value
    );

    inline auto make_history_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_history(
                context.request,
                context.db_connection_value,
                submission_id
            );
        };
    }

    inline auto make_detail_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_detail(
                context.request,
                context.db_connection_value,
                submission_id
            );
        };
    }

    inline auto make_create_request_guard(std::int64_t problem_id){
        return [problem_id](const http_guard::guard_context& context){
            return require_create_request(
                context.request,
                context.db_connection_value,
                problem_id
            );
        };
    }

    inline auto make_source_detail_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_source_detail(
                context.request,
                context.db_connection_value,
                submission_id
            );
        };
    }

    inline auto make_readable_source_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_readable_source(
                context.request,
                context.db_connection_value,
                submission_id
            );
        };
    }
}
