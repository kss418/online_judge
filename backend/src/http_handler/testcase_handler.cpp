#include "http_handler/testcase_handler.hpp"

#include "dto/problem_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "http_guard/testcase_upload_guard.hpp"

#include "db_service/testcase_service.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

testcase_handler::response_type testcase_handler::get_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::reference problem_reference_value{problem_id};
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
            const auto testcase_exp = testcase_service::get_testcase(
                db_connection_value,
                testcase_reference_value
            );
            return http_response_util::create_response_or_error(
                request,
                "get testcase",
                std::move(testcase_exp),
                [&](const request_type& error_request,
                    std::string_view action,
                    const service_error& code) {
                    if(code == service_error::not_found){
                        return http_response_util::create_not_found(error_request);
                    }

                    return http_response_util::create_4xx_or_500(
                        error_request,
                        action,
                        code
                    );
                },
                [&](const problem_dto::testcase& testcase_value) {
                    return http_response_util::create_json(
                        request,
                        boost::beast::http::status::ok,
                        problem_json_serializer::make_testcase_object(testcase_value)
                    );
                }
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

testcase_handler::response_type testcase_handler::get_testcases(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
            const auto testcase_summary_values_exp = testcase_service::list_testcase_summaries(
                db_connection_value,
                problem_reference_value
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "list testcases",
                std::move(testcase_summary_values_exp),
                problem_json_serializer::make_testcase_summary_list_object
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

testcase_handler::response_type testcase_handler::post_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&, const problem_dto::testcase& testcase_value) -> response_type {
            const auto create_testcase_exp = testcase_service::create_testcase(
                db_connection_value,
                problem_reference_value,
                testcase_value
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "create testcase",
                std::move(create_testcase_exp),
                problem_json_serializer::make_testcase_created_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_dto::testcase>(
            problem_request_parser::parse_testcase
        )
    );
}

testcase_handler::response_type testcase_handler::put_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&, const problem_dto::testcase& testcase_value) -> response_type {
            const auto set_testcase_exp = testcase_service::set_testcase(
                db_connection_value,
                testcase_reference_value,
                testcase_value
            );
            return http_response_util::create_response_or_4xx_or_500(
                request,
                "set testcase",
                std::move(set_testcase_exp),
                [&]() -> response_type {
                    return http_response_util::create_json_or_4xx_or_500(
                        request,
                        "get testcase",
                        testcase_service::get_testcase(
                            db_connection_value,
                            testcase_reference_value
                        ),
                        problem_json_serializer::make_testcase_object
                    );
                }
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_dto::testcase>(
            problem_request_parser::parse_testcase
        )
    );
}

testcase_handler::response_type testcase_handler::post_testcase_zip(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&,
            const std::vector<problem_dto::testcase>& testcase_values) -> response_type {
            const auto replace_testcases_exp = testcase_service::replace_testcases(
                db_connection_value,
                problem_reference_value,
                testcase_values
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "replace testcases",
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
        problem_guard::make_exists_guard(problem_reference_value),
        testcase_upload_guard::make_testcase_zip_guard()
    );
}

testcase_handler::response_type testcase_handler::move_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&,
            const problem_dto::testcase_move_request& testcase_move_request) -> response_type {
            problem_dto::testcase_ref testcase_reference_value{
                .problem_id = problem_id,
                .testcase_order = testcase_move_request.source_testcase_order
            };
            const auto move_testcase_exp = testcase_service::move_testcase(
                db_connection_value,
                testcase_reference_value,
                testcase_move_request.target_testcase_order
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "move testcase",
                std::move(move_testcase_exp),
                "problem testcase moved"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value),
        request_guard::make_json_guard<problem_dto::testcase_move_request>(
            problem_request_parser::parse_testcase_move_request
        )
    );
}

testcase_handler::response_type testcase_handler::delete_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = testcase_order
    };
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
            const auto delete_testcase_exp = testcase_service::delete_testcase(
                db_connection_value,
                testcase_reference_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "delete testcase",
                std::move(delete_testcase_exp),
                "problem testcase deleted"
            );
        },
        auth_guard::make_admin_guard()
    );
}

testcase_handler::response_type testcase_handler::delete_all_testcases(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
            const auto delete_all_testcases_exp = testcase_service::delete_all_testcases(
                db_connection_value,
                problem_reference_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "delete all testcases",
                std::move(delete_all_testcases_exp),
                "problem testcases deleted"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
