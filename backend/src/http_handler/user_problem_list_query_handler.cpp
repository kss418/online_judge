#include "http_handler/user_problem_list_query_handler.hpp"

#include "application/list_user_solved_problems_query.hpp"
#include "application/list_user_wrong_problems_query.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

#include <optional>
#include <utility>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

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
}

response_type user_query_handler::get_me_solved_problems(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_solved_problems_spec());
}

response_type user_query_handler::get_me_wrong_problems(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_wrong_problems_spec());
}

response_type user_query_handler::get_user_solved_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_solved_problems_spec(user_id)
    );
}

response_type user_query_handler::get_user_wrong_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_wrong_problems_spec(user_id)
    );
}
