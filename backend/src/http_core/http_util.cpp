#include "http_core/http_util.hpp"

#include "common/permission_util.hpp"
#include "common/string_util.hpp"
#include "db_service/problem_core_service.hpp"

#include <string>
#include <utility>

#include <boost/beast/core/string.hpp>
#include <boost/beast/http/field.hpp>
std::optional<boost::json::object> http_util::parse_json_object(
    const request_type& request
){
    boost::system::error_code ec;
    auto request_value = boost::json::parse(request.body(), ec);
    if(ec || !request_value.is_object()){
        return std::nullopt;
    }

    return std::move(request_value.as_object());
}

std::optional<std::vector<std::string_view>> http_util::parse_path(
    std::string_view prefix,
    std::string_view path
){
    if(!path.starts_with(prefix)){
        return std::nullopt;
    }

    path.remove_prefix(prefix.size());
    if(!path.empty() && path.front() != '/'){
        return std::nullopt;
    }
    if(path.starts_with("/")){
        path.remove_prefix(1);
    }

    std::vector<std::string_view> path_segments;
    while(!path.empty()){
        const std::size_t slash_position = path.find('/');
        if(slash_position == std::string_view::npos){
            path_segments.push_back(path);
            break;
        }

        path_segments.push_back(path.substr(0, slash_position));
        path.remove_prefix(slash_position + 1);
    }

    return path_segments;
}

std::string_view http_util::get_target_path(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return target;
    }

    return target.substr(0, query_position);
}

std::optional<std::string_view> http_util::get_target_query(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return std::nullopt;
    }

    return target.substr(query_position + 1);
}

std::optional<std::vector<http_util::query_param>> http_util::parse_query_params(
    std::string_view query
){
    std::vector<query_param> query_params;
    if(query.empty()){
        return query_params;
    }

    while(true){
        const auto ampersand_position = query.find('&');
        const auto query_segment =
            ampersand_position == std::string_view::npos
                ? query
                : query.substr(0, ampersand_position);

        const auto equal_position = query_segment.find('=');
        if(
            query_segment.empty() ||
            equal_position == std::string_view::npos ||
            equal_position == 0 ||
            equal_position + 1 >= query_segment.size()
        ){
            return std::nullopt;
        }

        query_params.push_back(query_param{
            .key = query_segment.substr(0, equal_position),
            .value = query_segment.substr(equal_position + 1)
        });

        if(ampersand_position == std::string_view::npos){
            break;
        }

        query.remove_prefix(ampersand_position + 1);
    }

    return query_params;
}

std::expected<submission_dto::list_filter, http_util::response_type>
http_util::parse_submission_list_filter_or_400(
    const request_type& request
){
    return parse_query_dto_or_400<submission_dto::list_filter>(
        request,
        submission_dto::make_list_filter_from_query_params
    );
}

std::expected<problem_dto::list_filter, http_util::response_type>
http_util::parse_problem_list_filter_or_400(
    const request_type& request
){
    return parse_query_dto_or_400<problem_dto::list_filter>(
        request,
        problem_dto::make_list_filter_from_query_params
    );
}

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

std::expected<auth_dto::token, http_util::response_type> http_util::parse_bearer_token_or_401(
    const request_type& request
){
    const auto token_opt = get_bearer_token(request);
    if(!token_opt){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "missing_or_invalid_bearer_token",
            "missing or invalid bearer token"
        ));
    }

    auth_dto::token token_value;
    token_value.value = std::string{*token_opt};
    return token_value;
}

std::expected<auth_dto::identity, http_util::response_type> http_util::try_auth_bearer(
    const request_type& request,
    db_connection& db_connection
){
    const auto token_exp = parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::unexpected(std::move(token_exp.error()));
    }

    const auto auth_identity_exp = auth_service::auth_token(db_connection, *token_exp);
    if(!auth_identity_exp){
        const auto code = auth_identity_exp.error();
        if(code == errno_error::invalid_argument){
            return std::unexpected(http_response_util::create_bearer_error(
                request,
                "missing_or_invalid_bearer_token",
                "missing or invalid bearer token"
            ));
        }

        return std::unexpected(http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to authenticate token: " + to_string(code)
        ));
    }
    if(!auth_identity_exp->has_value()){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "invalid_or_expired_token",
            "invalid, expired, or revoked token"
        ));
    }

    return auth_identity_exp->value();
}

std::expected<std::optional<auth_dto::identity>, http_util::response_type>
http_util::try_optional_auth_bearer(
    const request_type& request,
    db_connection& db_connection
){
    const auto authorization_field_it = request.find(boost::beast::http::field::authorization);
    if(authorization_field_it == request.end()){
        return std::optional<auth_dto::identity>{};
    }

    const auto auth_identity_exp = try_auth_bearer(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }

    return std::optional<auth_dto::identity>{*auth_identity_exp};
}

std::expected<auth_dto::identity, http_util::response_type> http_util::try_admin_auth_bearer(
    const request_type& request,
    db_connection& db_connection
){
    const auto auth_identity_exp = try_auth_bearer(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }
    if(!permission_util::has_admin_access(auth_identity_exp->permission_level)){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "admin_bearer_token_required",
            "admin bearer token required"
        ));
    }

    return *auth_identity_exp;
}

std::expected<auth_dto::identity, http_util::response_type> http_util::try_superadmin_auth_bearer(
    const request_type& request,
    db_connection& db_connection
){
    const auto auth_identity_exp = try_auth_bearer(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }
    if(!permission_util::has_superadmin_access(auth_identity_exp->permission_level)){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "superadmin_bearer_token_required",
            "superadmin bearer token required"
        ));
    }

    return *auth_identity_exp;
}

bool http_util::is_owner_or_admin(
    const auth_dto::identity& auth_identity_value,
    std::int64_t owner_user_id
){
    return permission_util::has_admin_access(auth_identity_value.permission_level) ||
        auth_identity_value.user_id == owner_user_id;
}

std::optional<std::string_view> http_util::get_bearer_token(
    const request_type& request
){
    const auto authorization_field_it = request.find(boost::beast::http::field::authorization);
    if(authorization_field_it == request.end()){
        return std::nullopt;
    }

    std::string_view authorization_value{
        authorization_field_it->value().data(),
        authorization_field_it->value().size()
    };
    authorization_value = string_util::trim_left_whitespace(authorization_value);
    authorization_value = string_util::trim_right_whitespace(authorization_value);

    if(
        authorization_value.size() <= 7 ||
        !boost::beast::iequals(authorization_value.substr(0, 6), "Bearer") ||
        authorization_value[6] != ' '
    ){
        return std::nullopt;
    }

    const std::string_view token = string_util::trim_left_whitespace(
        authorization_value.substr(7)
    );
    if(token.empty()){
        return std::nullopt;
    }

    return token;
}
