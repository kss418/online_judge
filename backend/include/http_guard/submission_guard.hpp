#pragma once

#include "dto/problem_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_request_dto.hpp"
#include "http_core/request_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "request_parser/submission_request_parser.hpp"

#include <cstdint>
#include <expected>
#include <utility>

namespace submission_guard{
    inline auto make_create_request_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context& composite_context,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    submission_internal_dto::create_submission_command,
                    http_guard::response_type
                > {
                auto submit_request_exp =
                    request_dto::parse_json_or_400<submission_request_dto::submit_request>(
                        composite_context.request(),
                        submission_request_parser::parse_submit_request
                    );
                if(!submit_request_exp){
                    return std::unexpected(std::move(submit_request_exp.error()));
                }

                if(auth_identity_value.user_id <= 0){
                    return std::unexpected(http_response_util::create_error(
                        composite_context.request(),
                        request_error::make_invalid_argument_error(
                            "user_id must be positive",
                            "user_id"
                        )
                    ));
                }

                if(problem_id <= 0){
                    return std::unexpected(http_response_util::create_error(
                        composite_context.request(),
                        request_error::make_invalid_argument_error(
                            "problem_id must be positive",
                            "problem_id"
                        )
                    ));
                }

                submission_internal_dto::create_submission_command command_value;
                command_value.user_id = auth_identity_value.user_id;
                command_value.problem_id = problem_id;
                command_value.source_value = std::move(*submit_request_exp);
                return command_value;
            },
            auth_guard::make_auth_guard(),
            problem_guard::make_exists_guard(problem_dto::reference{problem_id})
        );
    }
}
