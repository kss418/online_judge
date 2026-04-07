#pragma once

#include "judge_core/application/workspace_session.hpp"
#include "error/judge_error.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>

class workspace_manager{
public:
    static std::expected<workspace_manager, judge_error> create(
        std::filesystem::path source_root_path
    );

    workspace_manager(workspace_manager&& other) noexcept;
    workspace_manager& operator=(workspace_manager&& other) noexcept;
    ~workspace_manager();

    workspace_manager(const workspace_manager&) = delete;
    workspace_manager& operator=(const workspace_manager&) = delete;

    std::expected<workspace_session, judge_error> create(
        std::int64_t submission_id
    );

private:
    explicit workspace_manager(
        std::filesystem::path source_root_path
    );

    std::expected<std::filesystem::path, judge_error> make_workspace_path(
        std::int64_t submission_id
    );
    std::expected<std::filesystem::path, judge_error> prepare_workspace(
        std::int64_t submission_id
    );

    std::filesystem::path source_root_path_;
};
