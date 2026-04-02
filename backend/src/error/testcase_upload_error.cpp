#include "error/testcase_upload_error.hpp"

#include <utility>

namespace testcase_upload_error{
    http_error invalid_zip(std::string message){
        return http_error{
            http_error_code::invalid_testcase_zip,
            std::move(message)
        };
    }

    http_error empty_zip_body(){
        return invalid_zip("invalid testcase zip: request body is empty");
    }

    response_type create_response(
        const request_type& request,
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
                    invalid_zip()
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
                    invalid_zip(error_value.detail)
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
                    invalid_zip()
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
