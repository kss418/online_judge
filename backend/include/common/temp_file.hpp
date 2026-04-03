#pragma once

#include "common/unique_fd.hpp"
#include "error/io_error.hpp"

#include <expected>
#include <filesystem>
#include <string_view>

class temp_file{
    unique_fd fd_;
    std::filesystem::path path_;

    void remove_file() noexcept;
public:
    temp_file() noexcept = default;
    temp_file(unique_fd fd, std::filesystem::path path) noexcept;

    temp_file(const temp_file&) = delete;
    temp_file& operator=(const temp_file&) = delete;

    temp_file(temp_file&& other) noexcept;
    temp_file& operator=(temp_file&& other) noexcept;

    ~temp_file() noexcept;

    static std::expected<temp_file, io_error> create(std::string_view pattern);

    void close_fd() noexcept;
    std::expected<void, io_error> close_fd_checked() noexcept;
    int get_fd() const noexcept;
    const std::filesystem::path& get_path() const noexcept;
    explicit operator bool() const noexcept;
};
