#include "http_handler/testcase_bulk_command_handler.hpp"

#include "db_service/testcase_bulk_mutation_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/handler_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/testcase_upload_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    struct replace_testcases_request{
        problem_dto::reference problem_reference_value;
        std::vector<problem_dto::testcase> testcase_values;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_post_testcase_zip_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&,
                const std::vector<problem_dto::testcase>& testcase_values)
                -> command_expected<replace_testcases_request> {
                return replace_testcases_request{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .testcase_values = testcase_values
                };
            },
            [](auto& connection, const replace_testcases_request& request) {
                return testcase_bulk_mutation_service::replace_testcases(
                    connection,
                    request.problem_reference_value,
                    request.testcase_values
                );
            },
            http_handler_spec::make_json_message_serializer(
                "problem testcases uploaded",
                problem_json_serializer::make_testcase_count_message_object
            ),
            testcase_upload_guard::make_testcase_zip_guard()
        );
    }

    auto make_delete_all_testcases_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> std::expected<problem_dto::reference, response_type> {
                return problem_dto::reference{problem_id};
            },
            testcase_bulk_mutation_service::delete_all_testcases,
            http_handler_spec::make_json_message_serializer(
                "problem testcases deleted",
                problem_json_serializer::make_message_object
            )
        );
    }
}

response_type testcase_command_handler::post_testcase_zip(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_post_testcase_zip_spec(problem_id)
    );
}

response_type testcase_command_handler::delete_all_testcases(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_delete_all_testcases_spec(problem_id)
    );
}
