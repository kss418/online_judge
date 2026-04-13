#include "http_handler/testcase_command_handler.hpp"

#include "application/replace_testcases_action.hpp"
#include "application/testcase_action.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "http_guard/testcase_upload_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_post_testcase_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase& testcase_value)
                -> std::expected<
                    create_testcase_action::command,
                    testcase_command_handler::response_type
                > {
                return create_testcase_action::command{
                    .problem_reference_value = problem_reference_value,
                    .testcase_value = testcase_value
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::testcase>(
                problem_request_parser::parse_testcase
            ),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_post_testcase_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_testcase_guard(problem_id),
            .execute = [](testcase_command_handler::context_type& context,
                const create_testcase_action::command& command_value) {
                return create_testcase_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_versioned_testcase_created_object,
            .error_response = http_endpoint::default_error_response_factory{},
            .success_status = boost::beast::http::status::created
        };
    }

    auto make_put_testcase_guard(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        problem_dto::testcase_ref testcase_reference_value{
            .problem_id = problem_id,
            .testcase_order = testcase_order
        };
        return http_guard::make_composite_guard(
            [testcase_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase& testcase_value)
                -> std::expected<
                    update_testcase_action::command,
                    testcase_command_handler::response_type
                > {
                return update_testcase_action::command{
                    .testcase_reference_value = testcase_reference_value,
                    .testcase_value = testcase_value
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::testcase>(
                problem_request_parser::parse_testcase
            )
        );
    }

    auto make_put_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_endpoint::endpoint_spec{
            .parse = make_put_testcase_guard(problem_id, testcase_order),
            .execute = [](testcase_command_handler::context_type& context,
                const update_testcase_action::command& command_value) {
                return update_testcase_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_versioned_testcase_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_post_testcase_zip_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const std::vector<problem_dto::testcase>& testcase_values)
                -> std::expected<
                    replace_testcases_action::command,
                    testcase_command_handler::response_type
                > {
                return replace_testcases_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .testcase_values = testcase_values
                };
            },
            auth_guard::make_admin_guard(),
            testcase_upload_guard::make_testcase_zip_guard()
        );
    }

    auto make_post_testcase_zip_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_testcase_zip_guard(problem_id),
            .execute = [](testcase_command_handler::context_type& context,
                const replace_testcases_action::command& command_value) {
                return replace_testcases_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize =
                [](const problem_dto::testcase_count_mutation_result& testcase_count_value) {
                    return problem_json_serializer::make_testcase_count_message_object(
                        "problem testcases uploaded",
                        testcase_count_value
                    );
                },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_move_testcase_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase_move_request& testcase_move_request)
                -> std::expected<
                    move_testcase_action::command,
                    testcase_command_handler::response_type
                > {
                return move_testcase_action::command{
                    .testcase_reference_value = problem_dto::testcase_ref{
                        .problem_id = problem_id,
                        .testcase_order = testcase_move_request.source_testcase_order
                    },
                    .target_testcase_order = testcase_move_request.target_testcase_order
                };
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value),
            request_parse_guard::make_json_guard<problem_dto::testcase_move_request>(
                problem_request_parser::parse_testcase_move_request
            )
        );
    }

    auto make_move_testcase_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_move_testcase_guard(problem_id),
            .execute = [](testcase_command_handler::context_type& context,
                const move_testcase_action::command& command_value) {
                return move_testcase_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem testcase moved",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_delete_testcase_guard(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_guard::make_composite_guard(
            [problem_id, testcase_order](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    delete_testcase_action::command,
                    testcase_command_handler::response_type
                > {
                return delete_testcase_action::command{
                    .problem_id = problem_id,
                    .testcase_order = testcase_order
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_delete_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_endpoint::endpoint_spec{
            .parse = make_delete_testcase_guard(problem_id, testcase_order),
            .execute = [](testcase_command_handler::context_type& context,
                const delete_testcase_action::command& command_value) {
                return delete_testcase_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem testcase deleted",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_delete_all_testcases_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    delete_all_testcases_action::command,
                    testcase_command_handler::response_type
                > {
                return problem_reference_value;
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_delete_all_testcases_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_delete_all_testcases_guard(problem_id),
            .execute = [](testcase_command_handler::context_type& context,
                const delete_all_testcases_action::command& command_value) {
                return delete_all_testcases_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem testcases deleted",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

testcase_command_handler::response_type testcase_command_handler::post_testcase(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_post_testcase_spec(problem_id)
    );
}

testcase_command_handler::response_type testcase_command_handler::put_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    return http_endpoint::run_json(
        context,
        make_put_testcase_spec(problem_id, testcase_order)
    );
}

testcase_command_handler::response_type testcase_command_handler::post_testcase_zip(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_post_testcase_zip_spec(problem_id)
    );
}

testcase_command_handler::response_type testcase_command_handler::move_testcase(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_move_testcase_spec(problem_id)
    );
}

testcase_command_handler::response_type testcase_command_handler::delete_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    return http_endpoint::run_json(
        context,
        make_delete_testcase_spec(problem_id, testcase_order)
    );
}

testcase_command_handler::response_type testcase_command_handler::delete_all_testcases(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_delete_all_testcases_spec(problem_id)
    );
}
