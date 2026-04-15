#include "http_handler/testcase_item_command_handler.hpp"

#include "db_service/testcase_item_mutation_service.hpp"
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

    struct create_testcase_request{
        problem_dto::reference problem_reference_value;
        problem_dto::testcase testcase_value;
    };

    struct update_testcase_request{
        problem_dto::testcase_ref testcase_reference_value;
        problem_dto::testcase testcase_value;
    };

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

    auto make_post_testcase_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};

        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase& testcase_value)
                -> command_expected<create_testcase_request> {
                return create_testcase_request{
                    .problem_reference_value = problem_reference_value,
                    .testcase_value = testcase_value
                };
            },
            [](auto& context, const create_testcase_request& request) {
                return testcase_item_mutation_service::create_testcase(
                    context.db_connection_ref(),
                    request.problem_reference_value,
                    request.testcase_value
                );
            },
            problem_json_serializer::make_versioned_testcase_created_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::testcase>(
                problem_request_parser::parse_testcase
            ),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_put_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        problem_dto::testcase_ref testcase_reference_value{
            .problem_id = problem_id,
            .testcase_order = testcase_order
        };

        return http_endpoint::make_guarded_json_spec(
            [testcase_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::testcase& testcase_value)
                -> command_expected<update_testcase_request> {
                return update_testcase_request{
                    .testcase_reference_value = testcase_reference_value,
                    .testcase_value = testcase_value
                };
            },
            [](auto& context, const update_testcase_request& request) {
                return testcase_item_mutation_service::set_testcase_and_get(
                    context.db_connection_ref(),
                    request.testcase_reference_value,
                    request.testcase_value
                );
            },
            problem_json_serializer::make_versioned_testcase_object,
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::testcase>(
                problem_request_parser::parse_testcase
            )
        );
    }

    auto make_delete_testcase_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order
    ){
        return http_endpoint::make_guarded_json_spec(
            [problem_id, testcase_order](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<problem_dto::testcase_ref> {
                return problem_dto::testcase_ref{
                    .problem_id = problem_id,
                    .testcase_order = testcase_order
                };
            },
            http_endpoint::make_db_execute(
                testcase_item_mutation_service::delete_testcase
            ),
            make_problem_message_serializer("problem testcase deleted"),
            auth_guard::make_admin_guard()
        );
    }
}

response_type testcase_command_handler::post_testcase(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_post_testcase_spec(problem_id));
}

response_type testcase_command_handler::put_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    return http_endpoint::run_json(
        context,
        make_put_testcase_spec(problem_id, testcase_order)
    );
}

response_type testcase_command_handler::delete_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    return http_endpoint::run_json(
        context,
        make_delete_testcase_spec(problem_id, testcase_order)
    );
}
