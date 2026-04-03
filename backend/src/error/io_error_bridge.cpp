#include "error/io_error_bridge.hpp"

namespace io_error_bridge{
    error_code to_error_code(const io_error& error){
        switch(error.code){
            case io_error_code::invalid_argument:
                return error_code::create(errno_error::invalid_argument);
            case io_error_code::permission_denied:
                return error_code::create(errno_error::permission_denied);
            case io_error_code::not_found:
                return error_code::create(errno_error::file_not_found);
            case io_error_code::conflict:
                return error_code::create(errno_error::file_exists);
            case io_error_code::unavailable:
                return error_code::create(errno_error::resource_temporarily_unavailable);
            case io_error_code::internal:
                return error_code::create(errno_error::io_error);
        }

        return error_code::create(errno_error::io_error);
    }
}
