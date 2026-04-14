#include "http_handler/testcase_move_command_handler.hpp"

#include "application/testcase_action.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_problem_message_serializer(std::string_view message){
        return [message](const problem_dto::mutation_result& mutation_value) {
            return problem_json_serializer::make_message_object(
                message,
                mutation_value
            );
        };
    }

    auto make_move_testcase_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};

        return http_endpoint::make_guarded_json_spec(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase_move_request& testcase_move_request)
                -> command_expected<move_testcase_action::command> {
                return move_testcase_action::command{
                    .testcase_reference_value = problem_dto::testcase_ref{
                        .problem_id = problem_id,
                        .testcase_order = testcase_move_request.source_testcase_order
                    },
                    .target_testcase_order = testcase_move_request.target_testcase_order
                };
            },
            http_endpoint::make_db_execute(move_testcase_action::execute),
            make_problem_message_serializer("problem testcase moved"),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value),
            request_parse_guard::make_json_guard<problem_dto::testcase_move_request>(
                problem_request_parser::parse_testcase_move_request
            )
        );
    }
}

response_type testcase_command_handler::move_testcase(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_move_testcase_spec(problem_id));
}
