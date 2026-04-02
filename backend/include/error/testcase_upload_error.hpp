#pragma once

#include "common/testcase_zip_service.hpp"
#include "error/http_error.hpp"
#include "http_core/http_response_util.hpp"

#include <string>

namespace testcase_upload_error{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    http_error invalid_zip(std::string message = {});
    http_error empty_zip_body();
    response_type create_response(
        const request_type& request,
        const testcase_zip_service::error& error_value
    );
}
