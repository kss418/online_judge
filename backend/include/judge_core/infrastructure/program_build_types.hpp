#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace program_build{
    enum class source_language{
        cpp,
        python,
        java
    };

    enum class compile_failure_kind{
        user_compile_error,
        compile_resource_exceeded
    };

    enum class compile_resource_exceeded_reason{
        wall_clock,
        signaled,
        unknown
    };

    struct compile_run_result{
        int exit_code_ = 0;
        std::optional<int> termination_signal_ = std::nullopt;
        bool killed_by_wall_clock_ = false;
        std::string stdout_text_;
        std::string stderr_text_;
        std::size_t stdout_bytes_ = 0;
        std::size_t stderr_bytes_ = 0;
        std::size_t max_rss_kb_ = 0;
        std::size_t wall_time_ms_ = 0;
        std::size_t cpu_time_ms_ = 0;
    };

    struct compile_failure{
        compile_failure_kind kind_ = compile_failure_kind::user_compile_error;
        compile_resource_exceeded_reason resource_reason_ =
            compile_resource_exceeded_reason::unknown;
        compile_run_result run_result_;
    };

    struct build_artifact{
        source_language language_ = source_language::cpp;
        std::filesystem::path workspace_host_path_;
        std::filesystem::path entry_file_host_path_;
        std::string main_class_name_;
        std::optional<compile_failure> compile_failure_opt_;

        bool is_runnable() const noexcept{
            return !compile_failure_opt_.has_value();
        }

        bool is_user_compile_error() const noexcept{
            return compile_failure_opt_.has_value() &&
                compile_failure_opt_->kind_ ==
                    compile_failure_kind::user_compile_error;
        }

        bool is_compile_resource_exceeded() const noexcept{
            return compile_failure_opt_.has_value() &&
                compile_failure_opt_->kind_ ==
                    compile_failure_kind::compile_resource_exceeded;
        }
    };
}
