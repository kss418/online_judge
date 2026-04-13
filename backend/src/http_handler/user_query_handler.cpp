#include "http_handler/user_query_handler.hpp"

#include "application/get_public_user_list_query.hpp"
#include "application/get_user_list_query.hpp"
#include "application/get_user_submission_ban_status_query.hpp"
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

#include <string>

namespace{
    using context_type = user_query_handler::context_type;
    using response_type = user_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_me_spec(){
        return http_endpoint::make_json_spec(
            auth_guard::make_auth_guard(),
            [](context_type&, const auth_dto::identity& auth_identity_value) {
                return auth_identity_value;
            },
            user_json_serializer::make_me_object
        );
    }

    auto make_get_me_submission_statistics_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<get_user_submission_statistics_query::command> {
                return get_user_submission_statistics_query::command{
                    .user_id = auth_identity_value.user_id
                };
            },
            http_endpoint::make_db_execute(
                get_user_submission_statistics_query::execute
            ),
            user_json_serializer::make_submission_statistics_object,
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_submission_ban_status_spec(
        auto&& build_command,
        auto&& auth_guard_value
    ){
        return http_endpoint::make_guarded_json_spec(
            std::forward<decltype(build_command)>(build_command),
            http_endpoint::make_db_execute(
                get_user_submission_ban_status_query::execute
            ),
            user_json_serializer::make_submission_ban_status_object,
            std::forward<decltype(auth_guard_value)>(auth_guard_value)
        );
    }

    auto make_get_me_submission_ban_spec(){
        return make_get_submission_ban_status_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<get_user_submission_ban_status_query::command> {
                return get_user_submission_ban_status_query::command{
                    .user_id = auth_identity_value.user_id
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_user_problem_list_spec(
        auto&& build_command,
        auto&& execute,
        auto&& serialize
    ){
        return http_endpoint::make_guarded_json_spec(
            std::forward<decltype(build_command)>(build_command),
            http_endpoint::make_db_execute(
                std::forward<decltype(execute)>(execute)
            ),
            std::forward<decltype(serialize)>(serialize),
            auth_guard::make_optional_auth_guard()
        );
    }

    auto make_get_me_solved_problems_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<list_user_solved_problems_query::command> {
                return list_user_solved_problems_query::command{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            http_endpoint::make_db_execute(
                list_user_solved_problems_query::execute
            ),
            user_json_serializer::make_solved_problem_list_object,
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_me_wrong_problems_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<list_user_wrong_problems_query::command> {
                return list_user_wrong_problems_query::command{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            http_endpoint::make_db_execute(
                list_user_wrong_problems_query::execute
            ),
            user_json_serializer::make_wrong_problem_list_object,
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_public_user_list_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const user_dto::list_filter& filter_value)
                -> command_expected<get_public_user_list_query::command> {
                return get_public_user_list_query::command{
                    .filter_value = filter_value
                };
            },
            http_endpoint::make_db_execute(get_public_user_list_query::execute),
            user_json_serializer::make_public_list_object,
            request_parse_guard::make_query_guard<user_dto::list_filter>(
                user_request_parser::parse_list_filter
            )
        );
    }

    auto make_get_user_summary_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_json_spec(
            [user_id](const http_guard::guard_context&)
                -> command_expected<get_user_summary_query::command> {
                return get_user_summary_query::command{.user_id = user_id};
            },
            http_endpoint::make_db_execute(get_user_summary_query::execute),
            user_json_serializer::make_summary_object
        );
    }

    auto make_get_user_summary_by_login_id_spec(std::string_view user_login_id){
        return http_endpoint::make_guarded_json_spec(
            [user_login_id](const http_guard::guard_context&)
                -> command_expected<get_user_summary_by_login_id_query::command> {
                return get_user_summary_by_login_id_query::command{
                    .user_login_id = std::string{user_login_id}
                };
            },
            http_endpoint::make_db_execute(
                get_user_summary_by_login_id_query::execute
            ),
            user_json_serializer::make_summary_object
        );
    }

    auto make_get_user_submission_statistics_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_json_spec(
            [user_id](const http_guard::guard_context&)
                -> command_expected<get_user_submission_statistics_query::command> {
                return get_user_submission_statistics_query::command{
                    .user_id = user_id
                };
            },
            http_endpoint::make_db_execute(
                get_user_submission_statistics_query::execute
            ),
            user_json_serializer::make_submission_statistics_object
        );
    }

    auto make_get_user_solved_problems_spec(std::int64_t user_id){
        return make_user_problem_list_spec(
            [user_id](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> command_expected<list_user_solved_problems_query::command> {
                return list_user_solved_problems_query::command{
                    .user_id = user_id,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            list_user_solved_problems_query::execute,
            user_json_serializer::make_solved_problem_list_object
        );
    }

    auto make_get_user_wrong_problems_spec(std::int64_t user_id){
        return make_user_problem_list_spec(
            [user_id](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> command_expected<list_user_wrong_problems_query::command> {
                return list_user_wrong_problems_query::command{
                    .user_id = user_id,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            list_user_wrong_problems_query::execute,
            user_json_serializer::make_wrong_problem_list_object
        );
    }

    auto make_get_user_list_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<get_user_list_query::command> {
                return get_user_list_query::command{};
            },
            http_endpoint::make_db_execute(get_user_list_query::execute),
            user_json_serializer::make_list_object,
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_user_submission_ban_spec(std::int64_t user_id){
        return make_get_submission_ban_status_spec(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<get_user_submission_ban_status_query::command> {
                return get_user_submission_ban_status_query::command{
                    .user_id = user_id
                };
            },
            auth_guard::make_admin_guard()
        );
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
    return http_endpoint::run_json(context, make_get_me_submission_statistics_spec());
}

user_query_handler::response_type user_query_handler::get_me_submission_ban(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_submission_ban_spec());
}

user_query_handler::response_type user_query_handler::get_me_solved_problems(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_solved_problems_spec());
}

user_query_handler::response_type user_query_handler::get_me_wrong_problems(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_wrong_problems_spec());
}

user_query_handler::response_type user_query_handler::get_public_user_list(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_public_user_list_spec());
}

user_query_handler::response_type user_query_handler::get_user_summary(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(context, make_get_user_summary_spec(user_id));
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
