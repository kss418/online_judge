#pragma once
#include "error/error_code.hpp"

#include <vector>
#include <string>
#include <expected>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <optional>
#include <sys/resource.h>

namespace sandbox_runner{
    using path = std::filesystem::path;

    enum class policy_profile{
        compile,
        run
    };

    enum class mount_profile{
        default_profile,
        java_profile
    };

    struct run_options{
        path workspace_host_path;
        std::optional<path> input_path_opt = std::nullopt;
        std::chrono::milliseconds time_limit{0};
        std::int64_t memory_limit_mb = 0;
        policy_profile policy = policy_profile::run;
        mount_profile mounts = mount_profile::default_profile;
    };

    struct run_result{
        int exit_code_ = 0;
        std::vector<std::string> output_lines_;
        std::string stderr_text_;
        std::size_t max_rss_kb_ = 0;
        std::size_t elapsed_ms_ = 0;
        bool time_limit_exceeded_ = false;
        bool memory_limit_exceeded_ = false;
        bool output_exceeded_ = false;

        bool is_success() const noexcept { return exit_code_ == 0; }
    };

    struct wait_result{
        int status_ = 0;
        rusage usage_{};
        bool killed_by_wall_clock_ = false;
        std::size_t elapsed_ms_ = 0;
    };

    std::expected<void, error_code> startup_self_check();

    void invalidate_cached_artifacts() noexcept;

    void exec_child(
        const std::vector<std::string>& sandbox_command_args,
        int input_fd,
        int output_fd,
        int error_fd
    );

    std::expected <wait_result, error_code> wait_wall_clock(pid_t pid, std::chrono::milliseconds time_limit);

    std::expected<run_result, error_code> run(
        const std::vector<std::string>& command_args,
        const run_options& run_options_value
    );
}
