#pragma once

#include <filesystem>

namespace workspace_path_mapper{
    std::filesystem::path sandbox_workspace_path();
    std::filesystem::path make_sandbox_path(
        const std::filesystem::path& host_workspace_path,
        const std::filesystem::path& host_path
    );
}
