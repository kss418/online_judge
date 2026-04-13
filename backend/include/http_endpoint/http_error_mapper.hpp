#pragma once

#include "application/create_submission_action.hpp"
#include "application/get_submission_source_query.hpp"
#include "error/auth_error.hpp"
#include "error/problem_content_error.hpp"
#include "error/submission_error.hpp"
#include "http_core/http_adapter.hpp"

#include <optional>

namespace http_error_mapper{
    inline auto auth_login_error(){
        return [](const http_adapter::request_type& request,
            const service_error& error_value) {
            return http_adapter::error_or_4xx_or_500(
                request,
                error_value,
                auth_error::map_login_service_error
            );
        };
    }

    inline auto auth_token_error(){
        return [](const http_adapter::request_type& request,
            const service_error& error_value) {
            return http_adapter::error_or_4xx_or_500(
                request,
                error_value,
                auth_error::map_token_service_error
            );
        };
    }

    inline auto create_submission_error(){
        return [](const http_adapter::request_type& request,
            const create_submission_action::error& error_value) {
            return http_adapter::error_or_4xx_or_500(
                request,
                error_value,
                [](const create_submission_action::error& mapped_error_value)
                    -> std::optional<http_error> {
                    if(mapped_error_value.is_submission_banned()){
                        return submission_error::submission_banned();
                    }

                    return std::nullopt;
                },
                [](const create_submission_action::error& mapped_error_value)
                    -> const service_error& {
                    return mapped_error_value.service_error_value;
                }
            );
        };
    }

    inline auto submission_source_error(){
        return [](const http_adapter::request_type& request,
            const get_submission_source_query::error& error_value) {
            return http_adapter::error_or_4xx_or_500(
                request,
                error_value,
                [](const get_submission_source_query::error& mapped_error_value)
                    -> std::optional<http_error> {
                    if(mapped_error_value.is_source_access_denied()){
                        return submission_error::source_access_denied();
                    }

                    return std::nullopt;
                },
                [](const get_submission_source_query::error& mapped_error_value)
                    -> const service_error& {
                    return mapped_error_value.service_error_value;
                }
            );
        };
    }

    inline auto delete_sample_error(){
        return [](const http_adapter::request_type& request,
            const service_error& error_value) {
            if(
                error_value.code == service_error_code::validation_error &&
                error_value.message == "missing sample to delete"
            ){
                return http_adapter::error(
                    request,
                    problem_content_error::missing_sample_to_delete()
                );
            }

            return http_response_util::create_4xx_or_500(
                request,
                error_value
            );
        };
    }
}
