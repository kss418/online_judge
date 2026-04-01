#include "http_handler/user_handler.hpp"

#include "db_service/auth_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/user_service.hpp"
#include "db_service/user_statistics_service.hpp"
#include "common/permission_util.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/user_guard.hpp"
#include "http_core/request_dto.hpp"
#include "serializer/user_json_serializer.hpp"

user_handler::response_type user_handler::get_me(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_me_object(*auth_identity_exp)
    );
}

user_handler::response_type user_handler::get_me_submission_statistics(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto get_submission_statistics_exp =
        user_statistics_service::get_submission_statistics(
            db_connection_value,
            auth_identity_exp->user_id
        );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get user submission statistics",
        std::move(get_submission_statistics_exp),
        user_json_serializer::make_submission_statistics_object
    );
}

user_handler::response_type user_handler::get_me_submission_ban(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto get_submission_ban_status_exp = user_service::get_submission_ban_status(
        db_connection_value,
        auth_identity_exp->user_id
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get current user submission ban",
        std::move(get_submission_ban_status_exp),
        user_json_serializer::make_submission_ban_status_object,
        [&]{
            return http_response_util::create_user_not_found(request);
        }
    );
}

user_handler::response_type user_handler::get_me_solved_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto list_user_solved_problems_exp =
        problem_core_service::list_user_solved_problems(
            db_connection_value,
            auth_identity_exp->user_id,
            auth_identity_exp->user_id
        );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get current user solved problems",
        std::move(list_user_solved_problems_exp),
        user_json_serializer::make_solved_problem_list_object
    );
}

user_handler::response_type user_handler::get_me_wrong_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto list_user_wrong_problems_exp =
        problem_core_service::list_user_wrong_problems(
            db_connection_value,
            auth_identity_exp->user_id,
            auth_identity_exp->user_id
        );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get current user wrong problems",
        std::move(list_user_wrong_problems_exp),
        user_json_serializer::make_wrong_problem_list_object
    );
}

user_handler::response_type user_handler::get_public_user_list(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto filter_exp = request_dto::parse_query_or_400<user_dto::list_filter>(
        request,
        user_dto::make_list_filter_from_query_params
    );
    if(!filter_exp){
        return std::move(filter_exp.error());
    }

    const auto get_public_list_exp = user_service::get_public_list(
        db_connection_value,
        *filter_exp
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get public user list",
        std::move(get_public_list_exp),
        user_json_serializer::make_public_list_object
    );
}

user_handler::response_type user_handler::get_user_summary(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto required_user_summary_exp = user_guard::require_summary(
        request,
        db_connection_value,
        user_id
    );
    if(!required_user_summary_exp){
        return std::move(required_user_summary_exp.error());
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_summary_object(*required_user_summary_exp)
    );
}

user_handler::response_type user_handler::get_user_summary_by_login_id(
    const request_type& request,
    db_connection& db_connection_value,
    std::string_view user_login_id
){
    const auto user_summary_exp = user_guard::require_summary_by_login_id(
        request,
        db_connection_value,
        user_login_id
    );
    if(!user_summary_exp){
        return std::move(user_summary_exp.error());
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_summary_object(*user_summary_exp)
    );
}

user_handler::response_type user_handler::get_user_submission_statistics(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto user_summary_exp = user_guard::require_summary(
        request,
        db_connection_value,
        user_id
    );
    if(!user_summary_exp){
        return std::move(user_summary_exp.error());
    }

    const auto get_submission_statistics_exp =
        user_statistics_service::get_submission_statistics(
            db_connection_value,
            user_id
        );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get user submission statistics",
        std::move(get_submission_statistics_exp),
        user_json_serializer::make_submission_statistics_object
    );
}

user_handler::response_type user_handler::get_user_solved_problems(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const std::optional<auth_dto::identity>& auth_identity_opt,
            const user_dto::summary&) -> response_type {
            const auto list_user_solved_problems_exp =
                problem_core_service::list_user_solved_problems(
                    db_connection_value,
                    user_id,
                    auth_identity_opt.has_value()
                        ? std::optional<std::int64_t>{auth_identity_opt->user_id}
                        : std::nullopt
                );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "get user solved problems",
                std::move(list_user_solved_problems_exp),
                user_json_serializer::make_solved_problem_list_object
            );
        },
        auth_guard::make_optional_auth_guard(),
        user_guard::make_summary_guard(user_id)
    );
}

user_handler::response_type user_handler::get_user_wrong_problems(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const std::optional<auth_dto::identity>& auth_identity_opt,
            const user_dto::summary&) -> response_type {
            const auto list_user_wrong_problems_exp =
                problem_core_service::list_user_wrong_problems(
                    db_connection_value,
                    user_id,
                    auth_identity_opt.has_value()
                        ? std::optional<std::int64_t>{auth_identity_opt->user_id}
                        : std::nullopt
                );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "get user wrong problems",
                std::move(list_user_wrong_problems_exp),
                user_json_serializer::make_wrong_problem_list_object
            );
        },
        auth_guard::make_optional_auth_guard(),
        user_guard::make_summary_guard(user_id)
    );
}

user_handler::response_type user_handler::put_user_admin(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_exp = auth_guard::require_superadmin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto update_permission_level_exp = auth_service::update_permission_level(
        db_connection_value,
        user_id,
        permission_util::ADMIN
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "update permission level",
        std::move(update_permission_level_exp),
        [&]{
            return user_json_serializer::make_permission_object(
                user_id,
                permission_util::ADMIN
            );
        },
        [&]{
            return http_response_util::create_user_not_found(request);
        }
    );
}

user_handler::response_type user_handler::put_user_regular(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_exp = auth_guard::require_superadmin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto update_permission_level_exp = auth_service::update_permission_level(
        db_connection_value,
        user_id,
        permission_util::USER
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "update permission level",
        std::move(update_permission_level_exp),
        [&]{
            return user_json_serializer::make_permission_object(
                user_id,
                permission_util::USER
            );
        },
        [&]{
            return http_response_util::create_user_not_found(request);
        }
    );
}

user_handler::response_type user_handler::get_user_submission_ban(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_exp = auth_guard::require_admin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto get_submission_ban_status_exp = user_service::get_submission_ban_status(
        db_connection_value,
        user_id
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get user submission ban",
        std::move(get_submission_ban_status_exp),
        user_json_serializer::make_submission_ban_status_object,
        [&]{
            return http_response_util::create_user_not_found(request);
        }
    );
}

user_handler::response_type user_handler::post_user_submission_ban(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_exp = auth_guard::require_admin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto submission_ban_request_exp =
        request_dto::parse_json_or_400<user_dto::submission_ban_request>(
            request,
            user_dto::make_submission_ban_request_from_json
        );
    if(!submission_ban_request_exp){
        return std::move(submission_ban_request_exp.error());
    }

    const auto create_submission_ban_exp = user_service::create_submission_ban(
        db_connection_value,
        user_id,
        submission_ban_request_exp->duration_minutes
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "create user submission ban",
        std::move(create_submission_ban_exp),
        user_json_serializer::make_submission_ban_object,
        [&]{
            return http_response_util::create_user_not_found(request);
        },
        boost::beast::http::status::created
    );
}

user_handler::response_type user_handler::delete_user_submission_ban(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_exp = auth_guard::require_admin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto clear_submission_banned_until_exp =
        user_service::clear_submission_banned_until(
            db_connection_value,
            user_id
        );
    return http_response_util::create_message_or_4xx_or_500(
        request,
        "clear user submission ban",
        std::move(clear_submission_banned_until_exp),
        "user submission ban cleared",
        [&]{
            return http_response_util::create_user_not_found(request);
        }
    );
}

user_handler::response_type user_handler::get_user_list(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_exp = auth_guard::require_admin(
        request,
        db_connection_value
    );
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto user_list_exp = auth_service::get_user_list(db_connection_value);
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "get user list",
        std::move(user_list_exp),
        user_json_serializer::make_list_object
    );
}
