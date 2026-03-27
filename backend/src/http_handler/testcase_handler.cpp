#include "http_handler/testcase_handler.hpp"

#include "common/blocking_io.hpp"
#include "common/temp_dir.hpp"
#include "common/temp_file.hpp"
#include "common/zip_util.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/json_util.hpp"
#include "http_core/testcase_util.hpp"
#include "http_core/http_util.hpp"

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
            return http_response_util::create_4xx_or_500(
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
            return http_response_util::create_4xx_or_500(
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
            return http_response_util::create_4xx_or_500(
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

testcase_handler::response_type testcase_handler::post_testcase_zip(
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

        if(request.body().empty()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_testcase_zip",
                "invalid testcase zip: request body is empty"
            );
        }

        auto temp_zip_file_exp = temp_file::create("/tmp/oj_testcase_zip_XXXXXX");
        if(!temp_zip_file_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to create testcase zip temp file: " + to_string(temp_zip_file_exp.error())
            );
        }

        temp_file temp_zip_file = std::move(*temp_zip_file_exp);
        const auto write_body_exp = blocking_io::write_all(
            temp_zip_file.get_fd(),
            request.body()
        );
        if(!write_body_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to write testcase zip temp file: " + to_string(write_body_exp.error())
            );
        }

        const auto close_zip_file_exp = temp_zip_file.close_fd_checked();
        if(!close_zip_file_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to finalize testcase zip temp file: " + to_string(close_zip_file_exp.error())
            );
        }

        const auto list_zip_entries_exp = zip_util::list_entry_names(
            temp_zip_file.get_path()
        );
        if(!list_zip_entries_exp){
            if(list_zip_entries_exp.error() == errno_error::file_not_found){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::internal_server_error,
                    "internal_server_error",
                    "failed to inspect testcase zip: unzip command unavailable"
                );
            }
            if(list_zip_entries_exp.error().is_bad_request_error()){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::bad_request,
                    "invalid_testcase_zip",
                    "invalid testcase zip"
                );
            }
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to inspect testcase zip: " + to_string(list_zip_entries_exp.error())
            );
        }

        const auto archive_entries_exp = http_server::testcase_util::parse_testcase_archive_entries(
            *list_zip_entries_exp
        );
        if(!archive_entries_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_testcase_zip",
                archive_entries_exp.error()
            );
        }

        auto extraction_directory_exp = temp_dir::create(
            "/tmp/oj_testcase_zip_extract_XXXXXX"
        );
        if(!extraction_directory_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to create testcase extract directory: " +
                    to_string(extraction_directory_exp.error())
            );
        }

        temp_dir extraction_directory = std::move(*extraction_directory_exp);
        const auto unzip_archive_exp = zip_util::extract_to_directory(
            temp_zip_file.get_path(),
            extraction_directory.get_path()
        );
        if(!unzip_archive_exp){
            if(unzip_archive_exp.error() == errno_error::file_not_found){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::internal_server_error,
                    "internal_server_error",
                    "failed to extract testcase zip: unzip command unavailable"
                );
            }
            if(unzip_archive_exp.error().is_bad_request_error()){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::bad_request,
                    "invalid_testcase_zip",
                    "invalid testcase zip"
                );
            }
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to extract testcase zip: " + to_string(unzip_archive_exp.error())
            );
        }

        const auto testcase_values_exp = http_server::testcase_util::load_testcases_from_directory(
            extraction_directory.get_path(),
            *archive_entries_exp
        );
        if(!testcase_values_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to read extracted testcase files: " + to_string(testcase_values_exp.error())
            );
        }

        const auto replace_testcases_exp = testcase_service::replace_testcases(
            db_connection_value,
            problem_reference_value,
            *testcase_values_exp
        );
        if(!replace_testcases_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "replace testcases",
                replace_testcases_exp.error()
            );
        }

        boost::json::object response_object = json_util::make_message_object(
            "problem testcases uploaded"
        );
        response_object["testcase_count"] = replace_testcases_exp->testcase_count;
        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            response_object
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
            return http_response_util::create_4xx_or_500(
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
            return http_response_util::create_4xx_or_500(
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

testcase_handler::response_type testcase_handler::delete_all_testcases(
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

        const auto delete_all_testcases_exp = testcase_service::delete_all_testcases(
            db_connection_value,
            problem_reference_value
        );
        if(!delete_all_testcases_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "delete all testcases",
                delete_all_testcases_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_message_object("problem testcases deleted")
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
