#include "http_handler/testcase_query_handler.hpp"

#include "application/testcase_query.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_get_testcase_guard(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_id, testcase_order](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    get_testcase_query::command,
                    testcase_query_handler::response_type
                > {
                return get_testcase_query::command{
                    .problem_id = problem_id,
                    .testcase_order = testcase_order
                };
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_get_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_endpoint::endpoint_spec{
            .parse = make_get_testcase_guard(problem_id, testcase_order),
            .execute = [](testcase_query_handler::context_type& context,
                const get_testcase_query::command& command_value) {
                return get_testcase_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_testcase_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_testcases_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    list_testcase_summaries_query::command,
                    testcase_query_handler::response_type
                > {
                return problem_reference_value;
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_get_testcases_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_get_testcases_guard(problem_id),
            .execute = [](testcase_query_handler::context_type& context,
                const list_testcase_summaries_query::command& command_value) {
                return list_testcase_summaries_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_testcase_summary_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

testcase_query_handler::response_type testcase_query_handler::get_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    return http_endpoint::run_json(
        context,
        make_get_testcase_spec(problem_id, testcase_order)
    );
}

testcase_query_handler::response_type testcase_query_handler::get_testcases(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_get_testcases_spec(problem_id)
    );
}
