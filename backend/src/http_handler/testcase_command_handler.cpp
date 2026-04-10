#include "http_handler/testcase_command_handler.hpp"

#include "application/replace_testcases_action.hpp"
#include "db_service/testcase_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "http_guard/testcase_upload_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

testcase_command_handler::response_type testcase_command_handler::post_testcase(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::testcase& testcase_value) -> response_type {
            const auto create_testcase_exp = testcase_service::create_testcase(
                context_value.db_connection_ref(),
                problem_reference_value,
                testcase_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(create_testcase_exp),
                problem_json_serializer::make_testcase_created_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_parse_guard::make_json_guard<problem_dto::testcase>(
            problem_request_parser::parse_testcase
        ),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

testcase_command_handler::response_type testcase_command_handler::put_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        context,
        [testcase_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::testcase& testcase_value) -> response_type {
            const auto set_testcase_exp = testcase_service::set_testcase_and_get(
                context_value.db_connection_ref(),
                testcase_reference_value,
                testcase_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(set_testcase_exp),
                problem_json_serializer::make_testcase_object
            );
        },
        auth_guard::make_admin_guard(),
        request_parse_guard::make_json_guard<problem_dto::testcase>(
            problem_request_parser::parse_testcase
        )
    );
}

testcase_command_handler::response_type testcase_command_handler::post_testcase_zip(
    context_type& context,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const auth_dto::identity&,
            const std::vector<problem_dto::testcase>& testcase_values) -> response_type {
            replace_testcases_action::command command_value{
                .problem_reference_value = problem_dto::reference{problem_id},
                .testcase_values = testcase_values
            };
            const auto replace_testcases_exp = replace_testcases_action::execute(
                context_value.db_connection_ref(),
                command_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(replace_testcases_exp),
                [](const problem_dto::testcase_count& testcase_count) {
                    boost::json::object response_object =
                        common_json_serializer::make_message_object(
                            "problem testcases uploaded"
                        );
                    response_object["testcase_count"] = testcase_count.testcase_count;
                    return response_object;
                }
            );
        },
        auth_guard::make_admin_guard(),
        testcase_upload_guard::make_testcase_zip_guard()
    );
}

testcase_command_handler::response_type testcase_command_handler::move_testcase(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_id, problem_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::testcase_move_request& testcase_move_request) -> response_type {
            problem_dto::testcase_ref testcase_reference_value{
                .problem_id = problem_id,
                .testcase_order = testcase_move_request.source_testcase_order
            };
            const auto move_testcase_exp = testcase_service::move_testcase(
                context_value.db_connection_ref(),
                testcase_reference_value,
                testcase_move_request.target_testcase_order
            );
            return http_adapter::message(
                context_value.request,
                std::move(move_testcase_exp),
                "problem testcase moved"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value),
        request_parse_guard::make_json_guard<problem_dto::testcase_move_request>(
            problem_request_parser::parse_testcase_move_request
        )
    );
}

testcase_command_handler::response_type testcase_command_handler::delete_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        context,
        [testcase_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto delete_testcase_exp = testcase_service::delete_testcase(
                context_value.db_connection_ref(),
                testcase_reference_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(delete_testcase_exp),
                "problem testcase deleted"
            );
        },
        auth_guard::make_admin_guard()
    );
}

testcase_command_handler::response_type testcase_command_handler::delete_all_testcases(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto delete_all_testcases_exp = testcase_service::delete_all_testcases(
                context_value.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(delete_all_testcases_exp),
                "problem testcases deleted"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
