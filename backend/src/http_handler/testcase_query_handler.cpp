#include "http_handler/testcase_query_handler.hpp"

#include "db_service/testcase_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

testcase_query_handler::response_type testcase_query_handler::get_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::reference problem_reference_value{problem_id};
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        context,
        [testcase_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto testcase_exp = testcase_service::get_testcase(
                context_value.db_connection_ref(),
                testcase_reference_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(testcase_exp),
                [](const problem_dto::testcase& testcase_value) {
                    return problem_json_serializer::make_testcase_object(testcase_value);
                }
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

testcase_query_handler::response_type testcase_query_handler::get_testcases(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto testcase_summary_values_exp = testcase_service::list_testcase_summaries(
                context_value.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(testcase_summary_values_exp),
                problem_json_serializer::make_testcase_summary_list_object
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
