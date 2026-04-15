#include "http_handler/testcase_query_handler.hpp"

#include "db_service/testcase_query_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    using response_type = testcase_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        problem_dto::reference problem_reference_value{problem_id};
        return http_endpoint::make_guarded_json_spec(
            [problem_id, testcase_order](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<problem_dto::testcase_ref> {
                return problem_dto::testcase_ref{
                    .problem_id = problem_id,
                    .testcase_order = testcase_order
                };
            },
            http_endpoint::make_db_execute(testcase_query_service::get_testcase),
            problem_json_serializer::make_testcase_object,
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_get_testcases_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<problem_dto::reference> {
                return problem_reference_value;
            },
            http_endpoint::make_db_execute(
                testcase_query_service::list_testcase_summaries
            ),
            problem_json_serializer::make_testcase_summary_list_object,
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
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
