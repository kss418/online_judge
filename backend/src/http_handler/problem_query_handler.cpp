#include "http_handler/problem_query_handler.hpp"

#include "application/list_problems_query.hpp"
#include "db_service/problem_query_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    using response_type = problem_query_handler::response_type;

    struct problem_detail_request{
        problem_dto::reference problem_reference_value;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_problems_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context& context,
                const std::optional<auth_dto::identity>& auth_identity_opt,
                const problem_dto::list_filter& filter_value)
                -> command_expected<list_problems_query::command> {
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
            http_endpoint::make_db_execute(list_problems_query::execute),
            [](const list_problems_query::result& result_value) {
                return problem_json_serializer::make_list_object(
                    result_value.summary_values,
                    result_value.total_problem_count
                );
            },
            auth_guard::make_optional_auth_guard(),
            request_parse_guard::make_problem_list_filter_guard()
        );
    }

    auto make_get_problem_spec(std::int64_t problem_id){
        return http_endpoint::make_guarded_json_spec(
            [problem_id](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt)
                -> command_expected<problem_detail_request> {
                return problem_detail_request{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            http_endpoint::make_db_execute(
                [](auto& connection, const problem_detail_request& request) {
                    return problem_query_service::get_problem_detail(
                        connection,
                        request.problem_reference_value,
                        request.viewer_user_id_opt
                    );
                }
            ),
            [](const problem_dto::detail& problem_detail) {
                return problem_json_serializer::make_detail_object(problem_detail);
            },
            auth_guard::make_optional_auth_guard()
        );
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
