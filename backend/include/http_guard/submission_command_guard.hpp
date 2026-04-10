#pragma once

#include "dto/submission_dto.hpp"
#include "http_core/request_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "request_parser/submission_request_parser.hpp"

#include <cstdint>
#include <expected>
#include <utility>

namespace submission_command_guard{
    inline auto make_create_request_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context& composite_context,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<submission_dto::create_request, http_guard::response_type> {
                auto create_request_exp =
                    request_dto::parse_json_or_400<submission_dto::create_request>(
                        composite_context.request(),
                        submission_request_parser::parse_create_request,
                        auth_identity_value.user_id,
                        problem_id
                    );
                if(!create_request_exp){
                    return std::unexpected(std::move(create_request_exp.error()));
                }

                return create_request_exp;
            },
            auth_guard::make_auth_guard()
        );
    }
}
