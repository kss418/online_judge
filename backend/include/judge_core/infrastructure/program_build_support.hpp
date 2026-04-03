#pragma once

#include "judge_core/infrastructure/program_build_types.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <utility>

namespace program_build_support{
    inline constexpr std::chrono::milliseconds compile_time_limit{30000};
    inline constexpr std::int64_t compile_memory_limit_mb = 1024;

    inline sandbox_runner::run_options make_compile_run_options(
        const std::filesystem::path& workspace_host_path,
        sandbox_runner::mount_profile mount_profile =
            sandbox_runner::mount_profile::default_profile
    ){
        sandbox_runner::run_options run_options_value;
        run_options_value.workspace_host_path = workspace_host_path;
        run_options_value.time_limit = compile_time_limit;
        run_options_value.memory_limit_mb = compile_memory_limit_mb;
        run_options_value.policy = sandbox_runner::policy_profile::compile;
        run_options_value.mounts = mount_profile;
        return run_options_value;
    }

    inline program_build::build_artifact make_build_artifact_base(
        program_build::source_language language,
        const std::filesystem::path& workspace_host_path
    ){
        program_build::build_artifact build_artifact_value;
        build_artifact_value.language_ = language;
        build_artifact_value.workspace_host_path_ = workspace_host_path;
        return build_artifact_value;
    }

    inline program_build::build_artifact make_compile_failure_artifact(
        program_build::source_language language,
        const std::filesystem::path& workspace_host_path,
        sandbox_runner::run_result compile_run_result_value
    ){
        auto build_artifact_value = make_build_artifact_base(
            language,
            workspace_host_path
        );
        build_artifact_value.compile_failure_opt_ = program_build::compile_failure{
            compile_run_result_value.exit_code_,
            std::move(compile_run_result_value.stderr_text_)
        };
        return build_artifact_value;
    }
}
