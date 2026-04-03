#pragma once

#include "error/error_code.hpp"
#include "error/io_error.hpp"

namespace io_error_bridge{
    error_code to_error_code(const io_error& error);
}
