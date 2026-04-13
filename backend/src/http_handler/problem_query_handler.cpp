#include "http_handler/problem_query_handler.hpp"

#include "application/get_problem_detail_query.hpp"
#include "application/list_problems_query.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_get_problems_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context& context,
                const std::optional<auth_dto::identity>& auth_identity_opt,
                const problem_dto::list_filter& filter_value)
                -> std::expected<
                    list_problems_query::command,
                    problem_query_handler::response_type
                > {
                list_problems_query::command command_value{
                    .filter_value = filter_value,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
                const auto filter_validation_exp =
                    problem_request_parser::validate_list_filter_for_viewer(
                        command_value.filter_value,
                        command_value.viewer_user_id_opt.has_value()
                    );
                if(!filter_validation_exp){
                    return std::unexpected(http_response_util::create_error(
                        context.request(),
                        filter_validation_exp.error()
                    ));
                }

                return command_value;
            },
            auth_guard::make_optional_auth_guard(),
            request_parse_guard::make_problem_list_filter_guard()
        );
    }

    auto make_get_problems_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_problems_guard(),
            .execute = [](problem_query_handler::context_type& context,
                const list_problems_query::command& command_value) {
                return list_problems_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const list_problems_query::result& result_value) {
                return problem_json_serializer::make_list_object(
                    result_value.summary_values,
                    result_value.total_problem_count
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_problem_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> std::expected<
                    get_problem_detail_query::command,
                    problem_query_handler::response_type
                > {
                return get_problem_detail_query::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            auth_guard::make_optional_auth_guard()
        );
    }

    auto make_get_problem_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_get_problem_guard(problem_id),
            .execute = [](problem_query_handler::context_type& context,
                const get_problem_detail_query::command& command_value) {
                return get_problem_detail_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::detail& problem_detail) {
                return problem_json_serializer::make_detail_object(problem_detail);
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

problem_query_handler::response_type problem_query_handler::get_problems(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_problems_spec());
}

problem_query_handler::response_type problem_query_handler::get_problem(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_get_problem_spec(problem_id)
    );
}
