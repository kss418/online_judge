#include "http_handler/user_handler.hpp"

#include "db_service/auth_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/user_service.hpp"
#include "db_service/user_statistics_service.hpp"
#include "common/permission_util.hpp"
#include "http_core/http_util.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    std::expected<user_dto::summary, user_handler::response_type> require_user_summary(
        const user_handler::request_type& request,
        db_connection& db_connection_value,
        std::int64_t user_id
    ){
        const auto get_user_summary_exp = user_service::get_summary(
            db_connection_value,
            user_id
        );
        if(!get_user_summary_exp){
            return std::unexpected(http_response_util::create_4xx_or_500(
                request,
                "get user summary",
                get_user_summary_exp.error()
            ));
        }
        if(!get_user_summary_exp->has_value()){
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            ));
        }

        return get_user_summary_exp->value();
    }

    std::expected<user_dto::summary, user_handler::response_type>
    require_user_summary_by_login_id(
        const user_handler::request_type& request,
        db_connection& db_connection_value,
        std::string_view user_login_id
    ){
        const auto get_user_summary_exp = user_service::get_summary_by_login_id(
            db_connection_value,
            user_login_id
        );
        if(!get_user_summary_exp){
            return std::unexpected(http_response_util::create_4xx_or_500(
                request,
                "get user summary by login id",
                get_user_summary_exp.error()
            ));
        }
        if(!get_user_summary_exp->has_value()){
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            ));
        }

        return get_user_summary_exp->value();
    }
}

user_handler::response_type user_handler::get_me(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                user_json_serializer::make_me_object(auth_identity_value)
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_me_submission_statistics(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto get_submission_statistics_exp =
                user_statistics_service::get_submission_statistics(
                    db_connection_value,
                    auth_identity_value.user_id
                );
            if(!get_submission_statistics_exp){
                return http_response_util::create_4xx_or_500(
                    request,
                    "get user submission statistics",
                    get_submission_statistics_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                user_json_serializer::make_submission_statistics_object(
                    *get_submission_statistics_exp
                )
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_me_solved_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto list_user_solved_problems_exp =
                problem_core_service::list_user_solved_problems(
                    db_connection_value,
                    auth_identity_value.user_id,
                    auth_identity_value.user_id
                );
            if(!list_user_solved_problems_exp){
                return http_response_util::create_4xx_or_500(
                    request,
                    "get current user solved problems",
                    list_user_solved_problems_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                user_json_serializer::make_solved_problem_list_object(
                    *list_user_solved_problems_exp
                )
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_me_wrong_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto list_user_wrong_problems_exp =
                problem_core_service::list_user_wrong_problems(
                    db_connection_value,
                    auth_identity_value.user_id,
                    auth_identity_value.user_id
                );
            if(!list_user_wrong_problems_exp){
                return http_response_util::create_4xx_or_500(
                    request,
                    "get current user wrong problems",
                    list_user_wrong_problems_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                user_json_serializer::make_wrong_problem_list_object(
                    *list_user_wrong_problems_exp
                )
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_public_user_list(
    const request_type& request,
    db_connection& db_connection_value
){
    const std::string_view target{
        request.target().data(),
        request.target().size()
    };
    const auto query_opt = http_util::get_target_query(target);
    const auto query_params_opt = http_util::parse_query_params(query_opt.value_or(""));
    if(!query_params_opt){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::bad_request,
            "invalid_query_string",
            "invalid query string"
        );
    }

    const auto filter_exp = user_dto::make_list_filter_from_query_params(*query_params_opt);
    if(!filter_exp){
        const auto& validation_error = filter_exp.error();
        return http_response_util::create_error(
            request,
            boost::beast::http::status::bad_request,
            validation_error.code,
            validation_error.message,
            validation_error.field_opt
        );
    }

    const auto get_public_list_exp = user_service::get_public_list(
        db_connection_value,
        *filter_exp
    );
    if(!get_public_list_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "get public user list",
            get_public_list_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_public_list_object(*get_public_list_exp)
    );
}

user_handler::response_type user_handler::get_user_summary(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto user_summary_exp = require_user_summary(
        request,
        db_connection_value,
        user_id
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

user_handler::response_type user_handler::get_user_summary_by_login_id(
    const request_type& request,
    db_connection& db_connection_value,
    std::string_view user_login_id
){
    const auto user_summary_exp = require_user_summary_by_login_id(
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
    const auto user_summary_exp = require_user_summary(
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
    if(!get_submission_statistics_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "get user submission statistics",
            get_submission_statistics_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_submission_statistics_object(
            *get_submission_statistics_exp
        )
    );
}

user_handler::response_type user_handler::get_user_solved_problems(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_opt_exp = http_util::try_optional_auth_bearer(
        request,
        db_connection_value
    );
    if(!auth_identity_opt_exp){
        return std::move(auth_identity_opt_exp.error());
    }

    const auto user_summary_exp = require_user_summary(
        request,
        db_connection_value,
        user_id
    );
    if(!user_summary_exp){
        return std::move(user_summary_exp.error());
    }

    const auto list_user_solved_problems_exp =
        problem_core_service::list_user_solved_problems(
            db_connection_value,
            user_id,
            auth_identity_opt_exp->has_value()
                ? std::optional<std::int64_t>{auth_identity_opt_exp->value().user_id}
                : std::nullopt
        );
    if(!list_user_solved_problems_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "get user solved problems",
            list_user_solved_problems_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_solved_problem_list_object(
            *list_user_solved_problems_exp
        )
    );
}

user_handler::response_type user_handler::get_user_wrong_problems(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto auth_identity_opt_exp = http_util::try_optional_auth_bearer(
        request,
        db_connection_value
    );
    if(!auth_identity_opt_exp){
        return std::move(auth_identity_opt_exp.error());
    }

    const auto user_summary_exp = require_user_summary(
        request,
        db_connection_value,
        user_id
    );
    if(!user_summary_exp){
        return std::move(user_summary_exp.error());
    }

    const auto list_user_wrong_problems_exp =
        problem_core_service::list_user_wrong_problems(
            db_connection_value,
            user_id,
            auth_identity_opt_exp->has_value()
                ? std::optional<std::int64_t>{auth_identity_opt_exp->value().user_id}
                : std::nullopt
        );
    if(!list_user_wrong_problems_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "get user wrong problems",
            list_user_wrong_problems_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        user_json_serializer::make_wrong_problem_list_object(
            *list_user_wrong_problems_exp
        )
    );
}

user_handler::response_type user_handler::put_user_admin(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_permission_level_exp = auth_service::update_permission_level(
            db_connection_value,
            user_id,
            permission_util::ADMIN
        );
        if(!update_permission_level_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "update permission level",
                update_permission_level_exp.error()
            );
        }
        if(!update_permission_level_exp.value()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            user_json_serializer::make_permission_object(user_id, permission_util::ADMIN)
        );
    };

    return http_util::with_superadmin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::put_user_regular(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_permission_level_exp = auth_service::update_permission_level(
            db_connection_value,
            user_id,
            permission_util::USER
        );
        if(!update_permission_level_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "update permission level",
                update_permission_level_exp.error()
            );
        }
        if(!update_permission_level_exp.value()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            user_json_serializer::make_permission_object(user_id, permission_util::USER)
        );
    };

    return http_util::with_superadmin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_user_list(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto user_list_exp = auth_service::get_user_list(db_connection_value);
        if(!user_list_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "get user list",
                user_list_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            user_json_serializer::make_list_object(*user_list_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
