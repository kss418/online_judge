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
}
