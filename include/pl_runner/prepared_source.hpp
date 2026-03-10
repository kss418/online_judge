#pragma once

#include "common/temp_file.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pl_runner{
    struct prepared_source{
        std::optional<temp_file> binary_file_;
        std::vector<std::string> run_command_args_;
        std::optional<sandbox_runner::run_result> compile_failed_run_result_;

        bool is_runnable() const noexcept{
            return !compile_failed_run_result_.has_value();
        }
    };

    inline prepared_source make_compile_failed_prepared_source(
        int exit_code,
        std::string stderr_text
    ){
        prepared_source prepared_source_value;
        sandbox_runner::run_result run_result_value;
        run_result_value.exit_code_ = exit_code;
        run_result_value.stderr_text_ = std::move(stderr_text);
        prepared_source_value.compile_failed_run_result_ = std::move(run_result_value);
        return prepared_source_value;
    }
}
