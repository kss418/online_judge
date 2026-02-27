#pragma once
#include "common/error_code.hpp"

#include <vector>
#include <string>
#include <expected>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <sys/resource.h>

namespace code_runner{
    using path = std::filesystem::path;

    struct run_result{
        int exit_code_ = 0;
        std::vector<std::string> output_lines_;
        std::string stderr_text_;
        std::size_t max_rss_kb_ = 0;
        std::size_t elapsed_ms_ = 0;
        bool time_limit_exceeded_ = false;
        bool memory_limit_exceeded_ = false;

        bool is_success() const noexcept { return exit_code_ == 0; }
    };

    struct wait_result{
        int status_ = 0;
        rusage usage_{};
        bool killed_by_wall_clock_ = false;
        std::size_t elapsed_ms_ = 0;
    };

    void exec_child(
        const path& binary_path, int input_fd, int output_fd, int error_fd,
        std::chrono::milliseconds time_limit, std::int64_t memory_limit_mb
    );

    std::expected <wait_result, error_code> wait_wall_clock(pid_t pid, std::chrono::milliseconds time_limit);

    std::expected<run_result, error_code> run_cpp(
        const path& binary_path, const path& input_path, std::chrono::milliseconds time_limit, std::int64_t memory_limit_mb
    );
};
