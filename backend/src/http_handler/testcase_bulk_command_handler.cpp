#include "http_handler/testcase_bulk_command_handler.hpp"

#include "application/replace_testcases_action.hpp"
#include "application/testcase_action.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/testcase_upload_guard.hpp"
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

    auto make_post_testcase_zip_spec(std::int64_t problem_id){
        return http_endpoint::make_guarded_json_spec(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const std::vector<problem_dto::testcase>& testcase_values)
                -> command_expected<replace_testcases_action::command> {
                return replace_testcases_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .testcase_values = testcase_values
                };
            },
            http_endpoint::make_db_execute(replace_testcases_action::execute),
            [](const problem_dto::testcase_count_mutation_result& testcase_count_value) {
                return problem_json_serializer::make_testcase_count_message_object(
                    "problem testcases uploaded",
                    testcase_count_value
                );
            },
            auth_guard::make_admin_guard(),
            testcase_upload_guard::make_testcase_zip_guard()
        );
    }

    auto make_delete_all_testcases_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};

        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<delete_all_testcases_action::command> {
                return problem_reference_value;
            },
            http_endpoint::make_db_execute(delete_all_testcases_action::execute),
            make_problem_message_serializer("problem testcases deleted"),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
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
