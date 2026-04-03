#pragma once

#include <filesystem>
#include <optional>
#include <string>

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

    struct build_artifact{
        source_language language_ = source_language::cpp;
        std::filesystem::path workspace_host_path_;
        std::filesystem::path entry_file_host_path_;
        std::string main_class_name_;
        std::optional<compile_failure> compile_failure_opt_;

        bool is_runnable() const noexcept{
            return !compile_failure_opt_.has_value();
        }
    };
}
