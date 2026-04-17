#include "http_handler/testcase_move_command_handler.hpp"

#include "db_service/testcase_item_mutation_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/admin_problem_spec_helper.hpp"
#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    struct move_testcase_request{
        problem_dto::testcase_ref testcase_reference_value;
        std::int32_t target_testcase_order = 0;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_move_testcase_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&,
                const problem_dto::testcase_move_request& testcase_move_request)
                -> command_expected<move_testcase_request> {
                return move_testcase_request{
                    .testcase_reference_value = problem_dto::testcase_ref{
                        .problem_id = problem_id,
                        .testcase_order = testcase_move_request.source_testcase_order
                    },
                    .target_testcase_order = testcase_move_request.target_testcase_order
                };
            },
            [](auto& db_connection, const move_testcase_request& request) {
                return testcase_item_mutation_service::move_testcase(
                    db_connection,
                    request.testcase_reference_value,
                    request.target_testcase_order
                );
            },
            http_handler_spec::make_json_message_serializer(
                "problem testcase moved",
                problem_json_serializer::make_message_object
            ),
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
