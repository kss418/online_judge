#include "http_core/http_util.hpp"

#include "common/permission_util.hpp"
#include "db_service/problem_core_service.hpp"

std::expected<void, http_util::response_type> http_util::require_existing_problem_or_response(
    const request_type& request,
    db_connection& db_connection,
    const problem_dto::reference& problem_reference_value
){
    const auto exists_problem_exp = problem_core_service::exists_problem(
        db_connection,
        problem_reference_value
    );
    if(!exists_problem_exp){
        return std::unexpected(http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to check problem: " + to_string(exists_problem_exp.error())
        ));
    }
    if(!exists_problem_exp->exists){
        return std::unexpected(http_response_util::create_problem_not_found(request));
    }

    return {};
}

bool http_util::is_owner_or_admin(
    const auth_dto::identity& auth_identity_value,
    std::int64_t owner_user_id
){
    return permission_util::has_admin_access(auth_identity_value.permission_level) ||
        auth_identity_value.user_id == owner_user_id;
}
