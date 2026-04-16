#include "http_handler/testcase_query_handler.hpp"

#include "db_service/testcase_query_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/admin_problem_spec_helper.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    using response_type = testcase_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [testcase_order](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<problem_dto::testcase_ref> {
                return problem_dto::testcase_ref{
                    .problem_id = problem_id,
                    .testcase_order = testcase_order
                };
            },
            testcase_query_service::get_testcase,
            problem_json_serializer::make_testcase_object
        );
    }

    auto make_get_testcases_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&, std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<problem_dto::reference> {
                return problem_dto::reference{problem_id};
            },
            testcase_query_service::list_testcase_summaries,
            problem_json_serializer::make_testcase_summary_list_object
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
