#pragma once

#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace program_build{
    enum class source_language{
        cpp,
        python,
        java
    };

    struct compile_failure{
        int exit_code_ = 0;
        std::string stderr_text_;
    };

    struct runner_build_result{
        std::filesystem::path entry_host_path_;
        std::string entry_name_;
        std::optional<compile_failure> compile_failure_opt_;

        bool is_runnable() const noexcept{
            return !compile_failure_opt_.has_value();
        }
    };

    struct build_artifact{
        source_language language_ = source_language::cpp;
        std::filesystem::path entry_host_path_;
        std::string entry_name_;
        std::optional<compile_failure> compile_failure_opt_;

        bool is_runnable() const noexcept{
            return !compile_failure_opt_.has_value();
        }
    };

    struct execution_plan{
        std::filesystem::path workspace_host_path_;
        std::vector<std::string> run_command_args_;
        sandbox_runner::mount_profile mount_profile_ =
            sandbox_runner::mount_profile::default_profile;
    };
}
