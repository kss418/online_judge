#include "judge_core/infrastructure/cpp_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/program_handler.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <chrono>
#include <utility>

namespace{
    class cpp_program_handler final : public program_handler{
    public:
        explicit cpp_program_handler(std::filesystem::path cpp_compiler_path) :
            cpp_compiler_path_(std::move(cpp_compiler_path)){}

        bool supports_source_file(
            const std::filesystem::path& source_file_path
        ) const override{
            return source_file_path.extension() == ".cpp";
        }

        program_build::source_language language() const noexcept override{
            return program_build::source_language::cpp;
        }

        std::expected<program_build::build_artifact, sandbox_error> build_source(
            const std::filesystem::path& source_file_path
        ) const override{
            constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
            constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;

            const auto workspace_host_path = source_file_path.parent_path();
            const auto binary_host_path = workspace_host_path / "program.out";
            const auto binary_sandbox_path = judge_workspace::make_sandbox_path(
                workspace_host_path,
                binary_host_path
            );
            const auto source_sandbox_path = judge_workspace::make_sandbox_path(
                workspace_host_path,
                source_file_path
            );
            if(
                cpp_compiler_path_.empty() ||
                binary_sandbox_path.empty() ||
                source_sandbox_path.empty()
            ){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            sandbox_runner::run_options compile_run_options_value;
            compile_run_options_value.workspace_host_path = workspace_host_path;
            compile_run_options_value.time_limit = COMPILE_TIME_LIMIT;
            compile_run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
            compile_run_options_value.policy = sandbox_runner::policy_profile::compile;

            const auto compile_run_exp = sandbox_runner::run(
                {
                    cpp_compiler_path_.string(),
                    "-std=c++23",
                    "-O2",
                    "-pipe",
                    source_sandbox_path.string(),
                    "-o",
                    binary_sandbox_path.string()
                },
                compile_run_options_value
            );
            if(!compile_run_exp){
                return std::unexpected(compile_run_exp.error());
            }

            program_build::build_artifact build_artifact_value;
            build_artifact_value.language_ = language();
            build_artifact_value.workspace_host_path_ = workspace_host_path;
            if(compile_run_exp->exit_code_ != 0){
                build_artifact_value.compile_failure_opt_ = program_build::compile_failure{
                    compile_run_exp->exit_code_,
                    std::move(compile_run_exp->stderr_text_)
                };
                return build_artifact_value;
            }

            build_artifact_value.entry_file_host_path_ = binary_host_path;
            return build_artifact_value;
        }

        std::expected<program_launch::execution_plan, sandbox_error>
        make_execution_plan(
            const program_build::build_artifact& build_artifact_value
        ) const override{
            if(
                build_artifact_value.language_ != language() ||
                !build_artifact_value.is_runnable() ||
                build_artifact_value.workspace_host_path_.empty() ||
                build_artifact_value.entry_file_host_path_.empty()
            ){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            const auto binary_sandbox_path = judge_workspace::make_sandbox_path(
                build_artifact_value.workspace_host_path_,
                build_artifact_value.entry_file_host_path_
            );
            if(binary_sandbox_path.empty()){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            program_launch::execution_plan execution_plan_value;
            execution_plan_value.workspace_host_path_ =
                build_artifact_value.workspace_host_path_;
            execution_plan_value.run_command_args_.push_back(
                binary_sandbox_path.string()
            );
            return execution_plan_value;
        }

    private:
        std::filesystem::path cpp_compiler_path_;
    };
}

std::unique_ptr<program_handler> make_cpp_program_handler(
    std::filesystem::path cpp_compiler_path
){
    return std::make_unique<cpp_program_handler>(std::move(cpp_compiler_path));
}
