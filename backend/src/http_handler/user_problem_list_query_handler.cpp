#include "http_handler/user_problem_list_query_handler.hpp"

#include "db_service/problem_query_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/user_guard.hpp"
#include "serializer/user_json_serializer.hpp"

#include <optional>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    struct user_problem_list_request{
        std::int64_t user_id = 0;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_me_solved_problems_spec(){
        return http_handler_spec::make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<user_problem_list_request> {
                return user_problem_list_request{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            [](auto& connection, const user_problem_list_request& request) {
                return problem_query_service::list_user_solved_problems(
                    connection,
                    request.user_id,
                    request.viewer_user_id_opt
                );
            },
            user_json_serializer::make_solved_problem_list_object
        );
    }

    auto make_get_me_wrong_problems_spec(){
        return http_handler_spec::make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<user_problem_list_request> {
                return user_problem_list_request{
                    .user_id = auth_identity_value.user_id,
                    .viewer_user_id_opt = auth_identity_value.user_id
                };
            },
            [](auto& connection, const user_problem_list_request& request) {
                return problem_query_service::list_user_wrong_problems(
                    connection,
                    request.user_id,
                    request.viewer_user_id_opt
                );
            },
            user_json_serializer::make_wrong_problem_list_object
        );
    }

    auto make_get_user_solved_problems_spec(std::int64_t user_id){
        return http_handler_spec::make_auth_optional_path_json_spec(
            user_id,
            [](const http_guard::guard_context&, std::int64_t user_id,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> command_expected<user_problem_list_request> {
                return user_problem_list_request{
                    .user_id = user_id,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            [](auto& connection, const user_problem_list_request& request) {
                return problem_query_service::list_user_solved_problems(
                    connection,
                    request.user_id,
                    request.viewer_user_id_opt
                );
            },
            user_json_serializer::make_solved_problem_list_object,
            user_guard::make_exists_guard(user_id)
        );
    }

    auto make_get_user_wrong_problems_spec(std::int64_t user_id){
        return http_handler_spec::make_auth_optional_path_json_spec(
            user_id,
            [](const http_guard::guard_context&, std::int64_t user_id,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> command_expected<user_problem_list_request> {
                return user_problem_list_request{
                    .user_id = user_id,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            [](auto& connection, const user_problem_list_request& request) {
                return problem_query_service::list_user_wrong_problems(
                    connection,
                    request.user_id,
                    request.viewer_user_id_opt
                );
            },
            user_json_serializer::make_wrong_problem_list_object,
            user_guard::make_exists_guard(user_id)
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
