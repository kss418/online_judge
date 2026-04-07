#pragma once

#include "error/judge_error.hpp"

#include <expected>
#include <filesystem>
#include <string_view>

class workspace_manager;

class workspace_session{
public:
    workspace_session(workspace_session&& other) noexcept;
    workspace_session& operator=(workspace_session&& other) noexcept;
    ~workspace_session();

    workspace_session(const workspace_session&) = delete;
    workspace_session& operator=(const workspace_session&) = delete;

    const std::filesystem::path& path() const noexcept;

    std::expected<std::filesystem::path, judge_error> write_source_file(
        std::string_view language,
        std::string_view source_code
    );

    std::expected<void, judge_error> close();

private:
    friend class workspace_manager;

    explicit workspace_session(std::filesystem::path workspace_path);

    std::filesystem::path workspace_path_;
    bool closed_ = false;
};
