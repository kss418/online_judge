#include "http_handler/user_problem_list_query_handler.hpp"

#include "db_service/problem_query_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    struct user_problem_list_request{
        std::int64_t user_id = 0;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    template <typename list_fn_type>
    auto make_user_problem_list_execute(list_fn_type&& list_fn){
        using list_fn_storage_type = std::decay_t<list_fn_type>;

        return [
            list_fn_value = list_fn_storage_type(
                std::forward<list_fn_type>(list_fn)
            )
        ](auto& connection, const user_problem_list_request& request) mutable {
            return std::invoke(
                list_fn_value,
                connection,
                request.user_id,
                request.viewer_user_id_opt
            );
        };
    }

    template <typename list_fn_type, typename serialize_type>
    auto make_get_me_user_problem_list_spec(
        list_fn_type&& list_fn,
        serialize_type&& serialize
    ){
        auto execute = make_user_problem_list_execute(
            std::forward<list_fn_type>(list_fn)
        );

        return http_handler_spec::make_auth_user_id_json_spec(
            [execute_value = std::move(execute)](auto& connection, std::int64_t user_id) mutable {
                return execute_value(
                    connection,
                    user_problem_list_request{
                        .user_id = user_id,
                        .viewer_user_id_opt = user_id
                    }
                );
            },
            std::forward<serialize_type>(serialize)
        );
    }

    template <typename list_fn_type, typename serialize_type>
    auto make_get_user_problem_list_spec(
        std::int64_t user_id,
        list_fn_type&& list_fn,
        serialize_type&& serialize
    ){
        return http_handler_spec::make_auth_optional_existing_user_json_spec(
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
            make_user_problem_list_execute(std::forward<list_fn_type>(list_fn)),
            std::forward<serialize_type>(serialize)
        );
    }
}

response_type user_query_handler::get_me_solved_problems(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_user_problem_list_spec(
            problem_query_service::list_user_solved_problems,
            user_json_serializer::make_solved_problem_list_object
        )
    );
}

response_type user_query_handler::get_me_wrong_problems(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_me_user_problem_list_spec(
            problem_query_service::list_user_wrong_problems,
            user_json_serializer::make_wrong_problem_list_object
        )
    );
}

response_type user_query_handler::get_user_solved_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_problem_list_spec(
            user_id,
            problem_query_service::list_user_solved_problems,
            user_json_serializer::make_solved_problem_list_object
        )
    );
}

response_type user_query_handler::get_user_wrong_problems(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_problem_list_spec(
            user_id,
            problem_query_service::list_user_wrong_problems,
            user_json_serializer::make_wrong_problem_list_object
        )
    );
}
