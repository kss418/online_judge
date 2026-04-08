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

    inline program_build::compile_run_result make_compile_run_result(
        sandbox_runner::run_result compile_run_result_value
    ){
        program_build::compile_run_result compile_run_value;
        compile_run_value.exit_code_ = compile_run_result_value.exit_code_;
        compile_run_value.termination_signal_ =
            compile_run_result_value.termination_signal_;
        compile_run_value.killed_by_wall_clock_ =
            compile_run_result_value.killed_by_wall_clock_;
        compile_run_value.stdout_text_ =
            std::move(compile_run_result_value.stdout_text_);
        compile_run_value.stderr_text_ =
            std::move(compile_run_result_value.stderr_text_);
        compile_run_value.stdout_bytes_ = compile_run_result_value.stdout_bytes_;
        compile_run_value.stderr_bytes_ = compile_run_result_value.stderr_bytes_;
        compile_run_value.max_rss_kb_ = compile_run_result_value.max_rss_kb_;
        compile_run_value.wall_time_ms_ = compile_run_result_value.wall_time_ms_;
        compile_run_value.cpu_time_ms_ = compile_run_result_value.cpu_time_ms_;
        return compile_run_value;
    }

    inline program_build::compile_resource_exceeded_reason
    classify_compile_resource_exceeded_reason(
        const sandbox_runner::run_result& compile_run_result_value
    ){
        if(compile_run_result_value.killed_by_wall_clock_){
            return program_build::compile_resource_exceeded_reason::wall_clock;
        }

        if(compile_run_result_value.termination_signal_.has_value()){
            return program_build::compile_resource_exceeded_reason::signaled;
        }

        return program_build::compile_resource_exceeded_reason::unknown;
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
        const auto resource_reason = classify_compile_resource_exceeded_reason(
            compile_run_result_value
        );

        program_build::compile_failure compile_failure_value;
        compile_failure_value.kind_ =
            resource_reason == program_build::compile_resource_exceeded_reason::unknown
                ? program_build::compile_failure_kind::user_compile_error
                : program_build::compile_failure_kind::compile_resource_exceeded;
        compile_failure_value.resource_reason_ = resource_reason;
        compile_failure_value.run_result_ = make_compile_run_result(
            std::move(compile_run_result_value)
        );
        build_artifact_value.compile_failure_opt_ =
            std::move(compile_failure_value);
        return build_artifact_value;
    }
}
