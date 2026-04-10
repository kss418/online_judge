#include "http_handler/user_handler.hpp"

#include "application/get_public_user_list_query.hpp"
#include "application/get_user_submission_statistics_query.hpp"
#include "application/get_user_summary_by_login_id_query.hpp"
#include "application/get_user_summary_query.hpp"
#include "application/list_user_solved_problems_query.hpp"
#include "application/list_user_wrong_problems_query.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/user_service.hpp"
#include "db_service/user_statistics_service.hpp"
#include "common/permission_util.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

user_handler::response_type user_handler::get_me(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            return http_adapter::json(
                context.request,
                user_json_serializer::make_me_object(auth_identity_value)
            );
        },
        auth_guard::make_auth_guard()
    );
}

user_handler::response_type user_handler::get_me_submission_statistics(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto get_submission_statistics_exp =
                user_statistics_service::get_submission_statistics(
                    context.db_connection_ref(),
                    auth_identity_value.user_id
                );
            return http_adapter::json(
                context.request,
                std::move(get_submission_statistics_exp),
                user_json_serializer::make_submission_statistics_object
            );
        },
        auth_guard::make_auth_guard()
    );
}

user_handler::response_type user_handler::get_me_submission_ban(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto get_submission_ban_status_exp = user_service::get_submission_ban_status(
                context.db_connection_ref(),
                auth_identity_value.user_id
            );
            return http_adapter::json(
                context.request,
                std::move(get_submission_ban_status_exp),
                user_json_serializer::make_submission_ban_status_object
            );
        },
        auth_guard::make_auth_guard()
    );
}

user_handler::response_type user_handler::get_me_solved_problems(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto list_user_solved_problems_exp =
                problem_core_service::list_user_solved_problems(
                    context.db_connection_ref(),
                    auth_identity_value.user_id,
                    auth_identity_value.user_id
                );
            return http_adapter::json(
                context.request,
                std::move(list_user_solved_problems_exp),
                user_json_serializer::make_solved_problem_list_object
            );
        },
        auth_guard::make_auth_guard()
    );
}

user_handler::response_type user_handler::get_me_wrong_problems(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto list_user_wrong_problems_exp =
                problem_core_service::list_user_wrong_problems(
                    context.db_connection_ref(),
                    auth_identity_value.user_id,
                    auth_identity_value.user_id
                );
            return http_adapter::json(
                context.request,
                std::move(list_user_wrong_problems_exp),
                user_json_serializer::make_wrong_problem_list_object
            );
        },
        auth_guard::make_auth_guard()
    );
}

user_handler::response_type user_handler::get_public_user_list(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const user_dto::list_filter& filter_value) -> response_type {
            get_public_user_list_query::command command_value{
                .filter_value = filter_value
            };
            const auto get_public_list_exp = get_public_user_list_query::execute(
                context.db_connection_ref(),
                command_value
            );
            return http_adapter::json(
                context.request,
                std::move(get_public_list_exp),
                user_json_serializer::make_public_list_object
            );
        },
        request_guard::make_query_guard<user_dto::list_filter>(
            user_request_parser::parse_list_filter
        )
    );
}

user_handler::response_type user_handler::get_user_summary(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const request_context&) -> response_type {
            return http_adapter::json(
                context.request,
                get_user_summary_query::execute(
                    context.db_connection_ref(),
                    get_user_summary_query::command{.user_id = user_id}
                ),
                user_json_serializer::make_summary_object
            );
        }
    );
}

user_handler::response_type user_handler::get_user_summary_by_login_id(
    context_type& context,
    std::string_view user_login_id
){
    return http_guard::run_or_respond(
        context,
        [&](const request_context&) -> response_type {
            return http_adapter::json(
                context.request,
                get_user_summary_by_login_id_query::execute(
                    context.db_connection_ref(),
                    get_user_summary_by_login_id_query::command{
                        .user_login_id = std::string{user_login_id}
                    }
                ),
                user_json_serializer::make_summary_object
            );
        }
    );
}

user_handler::response_type user_handler::get_user_submission_statistics(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const request_context&) -> response_type {
            const auto get_submission_statistics_exp =
                get_user_submission_statistics_query::execute(
                    context.db_connection_ref(),
                    get_user_submission_statistics_query::command{.user_id = user_id}
                );
            return http_adapter::json(
                context.request,
                std::move(get_submission_statistics_exp),
                user_json_serializer::make_submission_statistics_object
            );
        }
    );
}

user_handler::response_type user_handler::get_user_solved_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](context_type&,
            const std::optional<auth_dto::identity>& auth_identity_opt) -> response_type {
            const auto list_user_solved_problems_exp =
                list_user_solved_problems_query::execute(
                    context.db_connection_ref(),
                    list_user_solved_problems_query::command{
                        .user_id = user_id,
                        .viewer_user_id_opt = auth_identity_opt.has_value()
                            ? std::optional<std::int64_t>{auth_identity_opt->user_id}
                            : std::nullopt
                    }
                );
            return http_adapter::json(
                context.request,
                std::move(list_user_solved_problems_exp),
                user_json_serializer::make_solved_problem_list_object
            );
        },
        auth_guard::make_optional_auth_guard()
    );
}

user_handler::response_type user_handler::get_user_wrong_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](context_type&,
            const std::optional<auth_dto::identity>& auth_identity_opt) -> response_type {
            const auto list_user_wrong_problems_exp =
                list_user_wrong_problems_query::execute(
                    context.db_connection_ref(),
                    list_user_wrong_problems_query::command{
                        .user_id = user_id,
                        .viewer_user_id_opt = auth_identity_opt.has_value()
                            ? std::optional<std::int64_t>{auth_identity_opt->user_id}
                            : std::nullopt
                    }
                );
            return http_adapter::json(
                context.request,
                std::move(list_user_wrong_problems_exp),
                user_json_serializer::make_wrong_problem_list_object
            );
        },
        auth_guard::make_optional_auth_guard()
    );
}

user_handler::response_type user_handler::put_user_admin(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto update_permission_level_exp = auth_service::update_permission_level(
                context.db_connection_ref(),
                user_id,
                permission_util::ADMIN
            );
            return http_adapter::json(
                context.request,
                std::move(update_permission_level_exp),
                [&] {
                    return user_json_serializer::make_permission_object(
                        user_id,
                        permission_util::ADMIN
                    );
                }
            );
        },
        auth_guard::make_superadmin_guard()
    );
}

user_handler::response_type user_handler::put_user_regular(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto update_permission_level_exp = auth_service::update_permission_level(
                context.db_connection_ref(),
                user_id,
                permission_util::USER
            );
            return http_adapter::json(
                context.request,
                std::move(update_permission_level_exp),
                [&] {
                    return user_json_serializer::make_permission_object(
                        user_id,
                        permission_util::USER
                    );
                }
            );
        },
        auth_guard::make_superadmin_guard()
    );
}

user_handler::response_type user_handler::get_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto get_submission_ban_status_exp = user_service::get_submission_ban_status(
                context.db_connection_ref(),
                user_id
            );
            return http_adapter::json(
                context.request,
                std::move(get_submission_ban_status_exp),
                user_json_serializer::make_submission_ban_status_object
            );
        },
        auth_guard::make_admin_guard()
    );
}

user_handler::response_type user_handler::post_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&,
            const user_dto::submission_ban_request& submission_ban_request) -> response_type {
            const auto create_submission_ban_exp = user_service::create_submission_ban(
                context.db_connection_ref(),
                user_id,
                submission_ban_request.duration_minutes
            );
            return http_adapter::json(
                context.request,
                std::move(create_submission_ban_exp),
                user_json_serializer::make_submission_ban_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<user_dto::submission_ban_request>(
            user_request_parser::parse_submission_ban_request
        )
    );
}

user_handler::response_type user_handler::delete_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto clear_submission_banned_until_exp =
                user_service::clear_submission_banned_until(
                    context.db_connection_ref(),
                    user_id
                );
            return http_adapter::message(
                context.request,
                std::move(clear_submission_banned_until_exp),
                "user submission ban cleared"
            );
        },
        auth_guard::make_admin_guard()
    );
}

user_handler::response_type user_handler::get_user_list(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto user_list_exp = auth_service::get_user_list(context.db_connection_ref());
            return http_adapter::json(
                context.request,
                std::move(user_list_exp),
                user_json_serializer::make_list_object
            );
        },
        auth_guard::make_admin_guard()
    );
}
