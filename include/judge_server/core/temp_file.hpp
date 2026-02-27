#pragma once

#include "judge_server/core/error_code.hpp"
#include "judge_server/core/unique_fd.hpp"

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

    static std::expected<temp_file, error_code> create(std::string_view pattern);

    void close_fd() noexcept;
    int get_fd() const noexcept;
    const std::filesystem::path& get_path() const noexcept;
    explicit operator bool() const noexcept;
};
