#include "http_guard/testcase_upload_guard.hpp"

#include "common/testcase_zip_service.hpp"
#include "error/testcase_upload_error.hpp"

namespace{
    testcase_upload_guard::response_type make_testcase_zip_error_response(
        const testcase_upload_guard::request_type& request,
        const testcase_zip_service::error& error_value
    ){
        using error_kind = testcase_zip_service::error_kind;

        switch(error_value.kind){
            case error_kind::create_temp_file_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "create_testcase_zip_temp_file",
                    error_value.detail
                );
            case error_kind::write_temp_file_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "write_testcase_zip_temp_file",
                    error_value.detail
                );
            case error_kind::finalize_temp_file_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "finalize_testcase_zip_temp_file",
                    error_value.detail
                );
            case error_kind::inspect_zip_unavailable:
                return http_response_util::create_internal_server_error(
                    request,
                    "inspect_testcase_zip",
                    "unzip command unavailable"
                );
            case error_kind::inspect_zip_invalid:
                return http_response_util::create_error(
                    request,
                    testcase_upload_error::invalid_zip()
                );
            case error_kind::inspect_zip_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "inspect_testcase_zip",
                    error_value.detail
                );
            case error_kind::invalid_archive_entries:
                return http_response_util::create_error(
                    request,
                    testcase_upload_error::invalid_zip(error_value.detail)
                );
            case error_kind::create_extract_directory_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "create_testcase_extract_directory",
                    error_value.detail
                );
            case error_kind::extract_zip_unavailable:
                return http_response_util::create_internal_server_error(
                    request,
                    "extract_testcase_zip",
                    "unzip command unavailable"
                );
            case error_kind::extract_zip_invalid:
                return http_response_util::create_error(
                    request,
                    testcase_upload_error::invalid_zip()
                );
            case error_kind::extract_zip_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "extract_testcase_zip",
                    error_value.detail
                );
            case error_kind::load_testcases_failed:
                return http_response_util::create_internal_server_error(
                    request,
                    "load_extracted_testcases",
                    error_value.detail
                );
        }

        return http_response_util::create_internal_server_error(
            request,
            "process_testcase_zip"
        );
    }
}

std::expected<std::vector<problem_dto::testcase>, testcase_upload_guard::response_type>
testcase_upload_guard::require_testcase_zip_upload(const request_type& request){
    if(request.body().empty()){
        return std::unexpected(http_response_util::create_error(
            request,
            testcase_upload_error::empty_zip_body()
        ));
    }

    const auto testcase_values_exp = testcase_zip_service::load_testcases_from_zip_body(
        request.body()
    );
    if(!testcase_values_exp){
        return std::unexpected(make_testcase_zip_error_response(
            request,
            testcase_values_exp.error()
        ));
    }

    return std::move(*testcase_values_exp);
}
