#pragma once

#include "dto/auth_dto.hpp"
#include "dto/submission_dto.hpp"
#include "http_core/request_context.hpp"
#include "http_guard/guard_runner.hpp"

#include <cstdint>
#include <expected>

namespace submission_guard{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    std::expected<submission_dto::history_list, response_type> require_history(
        context_type& context,
        std::int64_t submission_id
    );
    std::expected<submission_dto::detail, response_type> require_detail(
        context_type& context,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, response_type> require_source_detail(
        context_type& context,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, response_type> require_readable_source(
        context_type& context,
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
                context.request_context_ref(),
                submission_id
            );
        };
    }

    inline auto make_detail_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_detail(
                context.request_context_ref(),
                submission_id
            );
        };
    }

    inline auto make_source_detail_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_source_detail(
                context.request_context_ref(),
                submission_id
            );
        };
    }

    inline auto make_readable_source_guard(std::int64_t submission_id){
        return [submission_id](const http_guard::guard_context& context){
            return require_readable_source(
                context.request_context_ref(),
                submission_id
            );
        };
    }
}
