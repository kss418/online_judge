#include "http_handler/testcase_handler.hpp"

#include "dto/problem_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/testcase_service.hpp"

#include <utility>

testcase_handler::response_type testcase_handler::get_testcases(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto exists_problem_exp = problem_core_service::exists_problem(
            db_connection_value,
            problem_reference_value
        );
        if(!exists_problem_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to check problem: " + to_string(exists_problem_exp.error())
            );
        }
        if(!exists_problem_exp->exists){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "problem_not_found",
                "problem not found"
            );
        }

        const auto testcase_values_exp = testcase_service::list_testcases(
            db_connection_value,
            problem_reference_value
        );
        if(!testcase_values_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to list testcases: " + to_string(testcase_values_exp.error())
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_problem_testcase_list_object(*testcase_values_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

testcase_handler::response_type testcase_handler::post_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto testcase_exp = http_util::parse_json_dto_or_400<problem_dto::testcase>(
            request,
            problem_dto::make_testcase_from_json
        );
        if(!testcase_exp){
            return std::move(testcase_exp.error());
        }

        const auto create_testcase_exp = testcase_service::create_testcase(
            db_connection_value,
            problem_reference_value,
            *testcase_exp
        );
        if(!create_testcase_exp){
            return http_response_util::create_400_or_500(
                request,
                "create testcase",
                create_testcase_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::created,
            json_util::make_problem_testcase_created_object(*create_testcase_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
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
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto testcase_exp = http_util::parse_json_dto_or_400<problem_dto::testcase>(
            request,
            problem_dto::make_testcase_from_json
        );
        if(!testcase_exp){
            return std::move(testcase_exp.error());
        }

        const auto set_testcase_exp = testcase_service::set_testcase(
            db_connection_value,
            testcase_reference_value,
            *testcase_exp
        );
        if(!set_testcase_exp){
            return http_response_util::create_400_or_500(
                request,
                "set testcase",
                set_testcase_exp.error()
            );
        }

        const auto updated_testcase_exp = testcase_service::get_testcase(
            db_connection_value,
            testcase_reference_value
        );
        if(!updated_testcase_exp){
            return http_response_util::create_400_or_500(
                request,
                "get testcase",
                updated_testcase_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_problem_testcase_object(*updated_testcase_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

testcase_handler::response_type testcase_handler::delete_testcase(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto testcase_count_exp = testcase_service::get_testcase_count(
            db_connection_value,
            problem_reference_value
        );
        if(!testcase_count_exp){
            return http_response_util::create_400_or_500(
                request,
                "get testcase count",
                testcase_count_exp.error()
            );
        }
        if(testcase_count_exp->testcase_count <= 0){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_testcase_delete_request",
                "failed to delete testcase: invalid argument"
            );
        }

        const auto delete_testcase_exp = testcase_service::delete_testcase(
            db_connection_value,
            problem_reference_value
        );
        if(!delete_testcase_exp){
            return http_response_util::create_400_or_500(
                request,
                "delete testcase",
                delete_testcase_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_message_object("problem testcase deleted")
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
