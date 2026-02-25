#pragma once
#include "core/error_code.hpp"

namespace blocking_io{
    std::expected <size_t, error_code> write_all(int fd, const std::string& text);
    std::expected <std::string, error_code> read_all(int fd);
};