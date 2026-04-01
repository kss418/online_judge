#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_guard/auth_guard.hpp"

#include <expected>
#include <functional>
#include <utility>

namespace problem_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<void, response_type> require_existing_problem_or_response(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value
    );

    template <typename callback_type>
    response_type with_existing_problem(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ){
        const auto require_problem_exp = require_existing_problem_or_response(
            request,
            db_connection,
            problem_reference_value
        );
        if(!require_problem_exp){
            return std::move(require_problem_exp.error());
        }

        return std::invoke(std::forward<callback_type>(callback));
    }

    template <typename callback_type>
    response_type with_existing_problem_admin(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ){
        auto&& callback_ref = callback;
        return auth_guard::with_admin_auth_bearer(
            request,
            db_connection,
            [&](const auth_dto::identity& auth_identity) -> response_type {
                const auto require_problem_exp = require_existing_problem_or_response(
                    request,
                    db_connection,
                    problem_reference_value
                );
                if(!require_problem_exp){
                    return std::move(require_problem_exp.error());
                }

                return std::invoke(callback_ref, auth_identity);
            }
        );
    }
}
