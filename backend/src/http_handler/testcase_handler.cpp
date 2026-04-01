#include "http_handler/testcase_handler.hpp"

#include "common/blocking_io.hpp"
#include "common/temp_dir.hpp"
#include "common/temp_file.hpp"
#include "common/zip_util.hpp"
#include "dto/problem_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "http_core/testcase_uploader.hpp"

#include "db_service/testcase_service.hpp"
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
                [&](const request_type& error_request, std::string_view action, const error_code& code) {
                    if(code == errno_error::invalid_argument){
                        return http_response_util::create_error(
                            error_request,
                            boost::beast::http::status::not_found,
                            "testcase_not_found",
                            "testcase not found"
                        );
                    }

                    return http_response_util::create_error(
                        error_request,
                        boost::beast::http::status::internal_server_error,
                        "internal_server_error",
                        "failed to " + std::string{action} + ": " + to_string(code)
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
            problem_dto::make_testcase_from_json
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
            problem_dto::make_testcase_from_json
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
        [&](const auth_dto::identity&) -> response_type {
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

            const auto archive_entries_exp = testcase_uploader::parse_testcase_archive_entries(
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

            const auto testcase_values_exp = testcase_uploader::load_testcases_from_directory(
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
        problem_guard::make_exists_guard(problem_reference_value)
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
            problem_dto::make_testcase_move_request_from_json
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
