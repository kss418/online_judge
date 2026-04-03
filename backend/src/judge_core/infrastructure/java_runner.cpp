#include "judge_core/infrastructure/java_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/program_handler.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <chrono>
#include <utility>

namespace{
    class java_program_handler final : public program_handler{
    public:
        java_program_handler(
            std::filesystem::path java_compiler_path,
            std::filesystem::path java_runtime_path
        ) :
            java_compiler_path_(std::move(java_compiler_path)),
            java_runtime_path_(std::move(java_runtime_path)){}

        bool supports_source_file(
            const std::filesystem::path& source_file_path
        ) const override{
            return source_file_path.extension() == ".java";
        }

        program_build::source_language language() const noexcept override{
            return program_build::source_language::java;
        }

        std::expected<program_build::build_artifact, sandbox_error> build_source(
            const std::filesystem::path& source_file_path
        ) const override{
            constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
            constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;

            const auto workspace_host_path = source_file_path.parent_path();
            const auto source_sandbox_path = judge_workspace::make_sandbox_path(
                workspace_host_path,
                source_file_path
            );
            if(java_compiler_path_.empty() || source_sandbox_path.empty()){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            sandbox_runner::run_options compile_run_options_value;
            compile_run_options_value.workspace_host_path = workspace_host_path;
            compile_run_options_value.time_limit = COMPILE_TIME_LIMIT;
            compile_run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
            compile_run_options_value.policy = sandbox_runner::policy_profile::compile;
            compile_run_options_value.mounts = sandbox_runner::mount_profile::java_profile;

            const auto compile_run_exp = sandbox_runner::run(
                {
                    java_compiler_path_.string(),
                    "-d",
                    judge_workspace::sandbox_workspace_path().string(),
                    source_sandbox_path.string()
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

            build_artifact_value.main_class_name_ = "Main";
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
                build_artifact_value.main_class_name_.empty() ||
                java_runtime_path_.empty()
            ){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            program_launch::execution_plan execution_plan_value;
            execution_plan_value.workspace_host_path_ =
                build_artifact_value.workspace_host_path_;
            execution_plan_value.mount_profile_ =
                sandbox_runner::mount_profile::java_profile;
            execution_plan_value.run_command_args_.push_back(
                java_runtime_path_.string()
            );
            execution_plan_value.run_command_args_.push_back("-XX:-UsePerfData");
            execution_plan_value.run_command_args_.push_back("-cp");
            execution_plan_value.run_command_args_.push_back(
                judge_workspace::sandbox_workspace_path().string()
            );
            execution_plan_value.run_command_args_.push_back(
                build_artifact_value.main_class_name_
            );
            return execution_plan_value;
        }

    private:
        std::filesystem::path java_compiler_path_;
        std::filesystem::path java_runtime_path_;
    };
}

std::unique_ptr<program_handler> make_java_program_handler(
    std::filesystem::path java_compiler_path,
    std::filesystem::path java_runtime_path
){
    return std::make_unique<java_program_handler>(
        std::move(java_compiler_path),
        std::move(java_runtime_path)
    );
}
