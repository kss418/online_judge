#include "judge_core/infrastructure/python_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/program_handler.hpp"

#include <utility>

namespace{
    class python_program_handler final : public program_handler{
    public:
        explicit python_program_handler(std::filesystem::path python_path) :
            python_path_(std::move(python_path)){}

        bool supports_source_file(
            const std::filesystem::path& source_file_path
        ) const override{
            return source_file_path.extension() == ".py";
        }

        program_build::source_language language() const noexcept override{
            return program_build::source_language::python;
        }

        std::expected<program_build::build_artifact, sandbox_error> build_source(
            const std::filesystem::path& source_file_path
        ) const override{
            if(source_file_path.empty()){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            program_build::build_artifact build_artifact_value;
            build_artifact_value.language_ = language();
            build_artifact_value.workspace_host_path_ = source_file_path.parent_path();
            build_artifact_value.entry_file_host_path_ = source_file_path;
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
                build_artifact_value.entry_file_host_path_.empty() ||
                python_path_.empty()
            ){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            const auto source_sandbox_path = judge_workspace::make_sandbox_path(
                build_artifact_value.workspace_host_path_,
                build_artifact_value.entry_file_host_path_
            );
            if(source_sandbox_path.empty()){
                return std::unexpected(sandbox_error::invalid_argument);
            }

            program_launch::execution_plan execution_plan_value;
            execution_plan_value.workspace_host_path_ =
                build_artifact_value.workspace_host_path_;
            execution_plan_value.run_command_args_.push_back(python_path_.string());
            execution_plan_value.run_command_args_.push_back(source_sandbox_path.string());
            return execution_plan_value;
        }

    private:
        std::filesystem::path python_path_;
    };
}

std::unique_ptr<program_handler> make_python_program_handler(
    std::filesystem::path python_path
){
    return std::make_unique<python_program_handler>(std::move(python_path));
}
