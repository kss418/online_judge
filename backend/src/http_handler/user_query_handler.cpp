#include "http_handler/user_query_handler.hpp"

#include "application/get_user_list_query.hpp"
#include "application/get_user_submission_ban_status_query.hpp"
#include "application/get_public_user_list_query.hpp"
#include "application/get_user_submission_statistics_query.hpp"
#include "application/get_user_summary_by_login_id_query.hpp"
#include "application/get_user_summary_query.hpp"
#include "application/list_user_solved_problems_query.hpp"
#include "application/list_user_wrong_problems_query.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    auto make_get_me_spec(){
        return http_endpoint::endpoint_spec{
            .parse = auth_guard::make_auth_guard(),
            .execute = [](user_query_handler::context_type&,
                const auth_dto::identity& auth_identity_value) {
                return auth_identity_value;
            },
            .serialize = user_json_serializer::make_me_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_me_submission_statistics_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    get_user_submission_statistics_query::command,
                    user_query_handler::response_type
                > {
                return get_user_submission_statistics_query::command{
                    .user_id = auth_identity_value.user_id
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_me_submission_statistics_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_me_submission_statistics_guard(),
            .execute = [](user_query_handler::context_type& context,
                const get_user_submission_statistics_query::command& command_value) {
                return get_user_submission_statistics_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_submission_statistics_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_me_submission_ban_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    get_user_submission_ban_status_query::command,
                    user_query_handler::response_type
                > {
                return get_user_submission_ban_status_query::command{
                    .user_id = auth_identity_value.user_id
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_me_submission_ban_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_me_submission_ban_guard(),
            .execute = [](user_query_handler::context_type& context,
                const get_user_submission_ban_status_query::command& command_value) {
                return get_user_submission_ban_status_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_submission_ban_status_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_me_solved_problems_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    list_user_solved_problems_query::command,
                    user_query_handler::response_type
                > {
                return list_user_solved_problems_query::command{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_me_solved_problems_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_me_solved_problems_guard(),
            .execute = [](user_query_handler::context_type& context,
                const list_user_solved_problems_query::command& command_value) {
                return list_user_solved_problems_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_solved_problem_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_me_wrong_problems_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    list_user_wrong_problems_query::command,
                    user_query_handler::response_type
                > {
                return list_user_wrong_problems_query::command{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_me_wrong_problems_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_me_wrong_problems_guard(),
            .execute = [](user_query_handler::context_type& context,
                const list_user_wrong_problems_query::command& command_value) {
                return list_user_wrong_problems_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_wrong_problem_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_public_user_list_spec(){
        return http_endpoint::endpoint_spec{
            .parse = request_parse_guard::make_query_guard<user_dto::list_filter>(
                user_request_parser::parse_list_filter
            ),
            .execute = [](user_query_handler::context_type& context,
                const user_dto::list_filter& filter_value) {
                return get_public_user_list_query::execute(
                    context.db_connection_ref(),
                    get_public_user_list_query::command{
                        .filter_value = filter_value
                    }
                );
            },
            .serialize = user_json_serializer::make_public_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_summary_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [user_id](user_query_handler::context_type& context,
                const http_endpoint::no_input&) {
                return get_user_summary_query::execute(
                    context.db_connection_ref(),
                    get_user_summary_query::command{.user_id = user_id}
                );
            },
            .serialize = user_json_serializer::make_summary_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_summary_by_login_id_spec(std::string_view user_login_id){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [user_login_id](user_query_handler::context_type& context,
                const http_endpoint::no_input&) {
                return get_user_summary_by_login_id_query::execute(
                    context.db_connection_ref(),
                    get_user_summary_by_login_id_query::command{
                        .user_login_id = std::string{user_login_id}
                    }
                );
            },
            .serialize = user_json_serializer::make_summary_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_submission_statistics_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [user_id](user_query_handler::context_type& context,
                const http_endpoint::no_input&) {
                return get_user_submission_statistics_query::execute(
                    context.db_connection_ref(),
                    get_user_submission_statistics_query::command{
                        .user_id = user_id
                    }
                );
            },
            .serialize = user_json_serializer::make_submission_statistics_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_user_problem_command_guard(
        std::int64_t user_id,
        auto execute_factory
    ){
        return http_guard::make_composite_guard(
            [user_id, execute_factory](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> std::expected<
                    decltype(execute_factory(std::optional<std::int64_t>{})),
                    user_query_handler::response_type
                > {
                return execute_factory(
                    auth_guard::get_viewer_user_id(auth_identity_opt)
                );
            },
            auth_guard::make_optional_auth_guard()
        );
    }

    auto make_get_user_solved_problems_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_user_problem_command_guard(
                user_id,
                [user_id](std::optional<std::int64_t> viewer_user_id_opt) {
                    return list_user_solved_problems_query::command{
                        .user_id = user_id,
                        .viewer_user_id_opt = viewer_user_id_opt
                    };
                }
            ),
            .execute = [](user_query_handler::context_type& context,
                const list_user_solved_problems_query::command& command_value) {
                return list_user_solved_problems_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_solved_problem_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_wrong_problems_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_user_problem_command_guard(
                user_id,
                [user_id](std::optional<std::int64_t> viewer_user_id_opt) {
                    return list_user_wrong_problems_query::command{
                        .user_id = user_id,
                        .viewer_user_id_opt = viewer_user_id_opt
                    };
                }
            ),
            .execute = [](user_query_handler::context_type& context,
                const list_user_wrong_problems_query::command& command_value) {
                return list_user_wrong_problems_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_wrong_problem_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_list_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    get_user_list_query::command,
                    user_query_handler::response_type
                > {
                return get_user_list_query::command{};
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_user_list_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_user_list_guard(),
            .execute = [](user_query_handler::context_type& context,
                const get_user_list_query::command& command_value) {
                return get_user_list_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_user_submission_ban_guard(std::int64_t user_id){
        return http_guard::make_composite_guard(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    get_user_submission_ban_status_query::command,
                    user_query_handler::response_type
                > {
                return get_user_submission_ban_status_query::command{
                    .user_id = user_id
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_get_user_submission_ban_guard(user_id),
            .execute = [](user_query_handler::context_type& context,
                const get_user_submission_ban_status_query::command& command_value) {
                return get_user_submission_ban_status_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_submission_ban_status_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

user_query_handler::response_type user_query_handler::get_me(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_spec());
}

user_query_handler::response_type user_query_handler::get_me_submission_statistics(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_submission_statistics_spec()
    );
}

user_query_handler::response_type user_query_handler::get_me_submission_ban(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_submission_ban_spec()
    );
}

user_query_handler::response_type user_query_handler::get_me_solved_problems(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_solved_problems_spec()
    );
}

user_query_handler::response_type user_query_handler::get_me_wrong_problems(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_wrong_problems_spec()
    );
}

user_query_handler::response_type user_query_handler::get_public_user_list(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_public_user_list_spec()
    );
}

user_query_handler::response_type user_query_handler::get_user_summary(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_summary_spec(user_id)
    );
}

user_query_handler::response_type user_query_handler::get_user_summary_by_login_id(
    context_type& context,
    std::string_view user_login_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_summary_by_login_id_spec(user_login_id)
    );
}

user_query_handler::response_type user_query_handler::get_user_submission_statistics(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_submission_statistics_spec(user_id)
    );
}

user_query_handler::response_type user_query_handler::get_user_solved_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_solved_problems_spec(user_id)
    );
}

user_query_handler::response_type user_query_handler::get_user_wrong_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_wrong_problems_spec(user_id)
    );
}

user_query_handler::response_type user_query_handler::get_user_list(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_user_list_spec());
}

user_query_handler::response_type user_query_handler::get_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_submission_ban_spec(user_id)
    );
}
