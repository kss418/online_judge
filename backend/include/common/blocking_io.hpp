#pragma once

#include "error/io_error.hpp"

#include <cstddef>
#include <expected>
#include <string>

namespace blocking_io{
    std::expected<size_t, io_error> write_all(int fd, const std::string& text);
    std::expected<std::string, io_error> read_all(int fd);
    std::expected<std::string, io_error> read_all_from_start(int fd);
};
