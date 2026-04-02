#pragma once

#include "error/error_code.hpp"

#include <expected>
#include <filesystem>
#include <string_view>

class temp_dir{
    std::filesystem::path path_;

    void remove_directory() noexcept;
public:
    temp_dir() noexcept = default;
    explicit temp_dir(std::filesystem::path path) noexcept;

    temp_dir(const temp_dir&) = delete;
    temp_dir& operator=(const temp_dir&) = delete;

    temp_dir(temp_dir&& other) noexcept;
    temp_dir& operator=(temp_dir&& other) noexcept;

    ~temp_dir() noexcept;

    static std::expected<temp_dir, error_code> create(std::string_view pattern);

    const std::filesystem::path& get_path() const noexcept;
    explicit operator bool() const noexcept;
};
