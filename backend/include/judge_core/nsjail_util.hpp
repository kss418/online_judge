#pragma once

#include "common/error_code.hpp"
#include "common/temp_dir.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sandbox_runner{
    enum class policy_profile;
    struct run_options;
}

namespace nsjail_util{
    struct sandbox_artifacts{
        temp_dir sandbox_dir_;
        std::filesystem::path rootfs_path_;
        std::filesystem::path seccomp_policy_path_;
        std::vector<std::string> fixed_mount_args_;
    };

    std::expected<std::filesystem::path, error_code> require_nsjail_path();

    std::expected<void, error_code> validate_workspace_path(
        const std::filesystem::path& workspace_host_path
    );

    std::expected<void, error_code> check_user_namespace_support();

    std::expected<std::shared_ptr<const sandbox_artifacts>, error_code> acquire_sandbox_artifacts(
        sandbox_runner::policy_profile policy_profile_value
    );

    void invalidate_all_sandbox_artifacts() noexcept;

    std::expected<std::vector<std::string>, error_code> make_command_args(
        const std::filesystem::path& nsjail_path,
        const sandbox_artifacts& sandbox_artifacts_value,
        const std::vector<std::string>& command_args,
        const sandbox_runner::run_options& run_options_value
    );
}
