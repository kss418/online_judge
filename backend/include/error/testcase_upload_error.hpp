#pragma once

#include "error/http_error.hpp"

#include <string>

namespace testcase_upload_error{
    http_error invalid_zip(std::string message = {});
    http_error empty_zip_body();
}
