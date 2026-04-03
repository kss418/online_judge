#pragma once

#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace program_launch{
    struct execution_plan{
        std::filesystem::path workspace_host_path_;
        std::vector<std::string> run_command_args_;
        sandbox_runner::mount_profile mount_profile_ =
            sandbox_runner::mount_profile::default_profile;
    };
}
