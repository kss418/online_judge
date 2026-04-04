#include "judge_core/application/execution_engine.hpp"

#include "common/env_util.hpp"
#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/infrastructure/launch_planner.hpp"
#include "judge_core/infrastructure/program_builder.hpp"
#include "judge_core/infrastructure/testcase_runner.hpp"
#include "judge_core/infrastructure/toolchain_config.hpp"

#include <memory>
#include <utility>

namespace{
    std::expected<std::filesystem::path, judge_error> load_required_path_env(
        const char* key,
        std::string empty_value_message
    ){
        const auto path_text_exp = env_util::require_env(key);
        if(!path_text_exp){
            return std::unexpected(judge_error{path_text_exp.error()});
        }

        const std::filesystem::path path_value(*path_text_exp);
        if(path_value.empty()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    std::move(empty_value_message)
                }
            );
        }

        return path_value;
    }

    std::expected<toolchain_config, judge_error> load_toolchain_config(){
        const auto cpp_compiler_path_exp = load_required_path_env(
            "JUDGE_CPP_COMPILER_PATH",
            "cpp compiler path is not configured"
        );
        if(!cpp_compiler_path_exp){
            return std::unexpected(cpp_compiler_path_exp.error());
        }

        const auto python_path_exp = load_required_path_env(
            "JUDGE_PYTHON_PATH",
            "python path is not configured"
        );
        if(!python_path_exp){
            return std::unexpected(python_path_exp.error());
        }

        const auto java_compiler_path_exp = load_required_path_env(
            "JUDGE_JAVA_COMPILER_PATH",
            "java compiler path is not configured"
        );
        if(!java_compiler_path_exp){
            return std::unexpected(java_compiler_path_exp.error());
        }

        const auto java_runtime_path_exp = load_required_path_env(
            "JUDGE_JAVA_RUNTIME_PATH",
            "java runtime path is not configured"
        );
        if(!java_runtime_path_exp){
            return std::unexpected(java_runtime_path_exp.error());
        }

        return toolchain_config{
            .cpp_compiler_path = std::move(*cpp_compiler_path_exp),
            .python_path = std::move(*python_path_exp),
            .java_compiler_path = std::move(*java_compiler_path_exp),
            .java_runtime_path = std::move(*java_runtime_path_exp)
        };
    }

    execution_report::testcase_execution make_testcase_execution(
        const program_build::compile_failure& compile_failure_value
    ){
        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = compile_failure_value.exit_code_;
        testcase_execution_value.stderr_text = compile_failure_value.stderr_text_;
        return testcase_execution_value;
    }

    execution_report::batch make_compile_failed_execution_report(
        const program_build::compile_failure& compile_failure_value
    ){
        execution_report::batch execution_report_value;
        execution_report_value.compile_failed = true;
        execution_report_value.executions.push_back(
            make_testcase_execution(compile_failure_value)
        );
        return execution_report_value;
    }
}

std::expected<execution_engine, judge_error> execution_engine::create(
    testcase_snapshot_port testcase_snapshot_port_value
){
    const auto toolchain_config_exp = load_toolchain_config();
    if(!toolchain_config_exp){
        return std::unexpected(toolchain_config_exp.error());
    }

    return execution_engine{
        std::make_unique<program_builder>(
            toolchain_config_exp->cpp_compiler_path,
            toolchain_config_exp->java_compiler_path
        ),
        std::make_unique<launch_planner>(
            toolchain_config_exp->python_path,
            toolchain_config_exp->java_runtime_path
        ),
        std::move(testcase_snapshot_port_value)
    };
}

execution_engine::execution_engine(
    std::unique_ptr<program_builder> program_builder_value,
    std::unique_ptr<launch_planner> launch_planner_value,
    testcase_snapshot_port testcase_snapshot_port_value
) :
    program_builder_(std::move(program_builder_value)),
    launch_planner_(std::move(launch_planner_value)),
    testcase_snapshot_port_(std::move(testcase_snapshot_port_value)){}

execution_engine::execution_engine(
    execution_engine&& other
) noexcept = default;

execution_engine& execution_engine::operator=(
    execution_engine&& other
) noexcept = default;

execution_engine::~execution_engine() = default;

std::expected<std::filesystem::path, judge_error>
execution_engine::write_submission_source(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    const auto source_file_path_exp = judge_workspace::write_source_file(
        workspace_path,
        queued_submission_value.language,
        queued_submission_value.source_code
    );
    if(!source_file_path_exp){
        return std::unexpected(judge_error{source_file_path_exp.error()});
    }

    return *source_file_path_exp;
}

std::expected<execution_engine::execute_result, judge_error> execution_engine::execute(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    const auto source_file_path_exp = write_submission_source(
        queued_submission_value,
        workspace_path
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    const auto build_source_exp = program_builder_->build_source(*source_file_path_exp);
    if(!build_source_exp){
        return std::unexpected(judge_error{build_source_exp.error()});
    }

    if(!build_source_exp->is_runnable()){
        auto process_submission_data_value =
            judge_submission_data::make_process_submission_data(
                judge_result::compile_error,
                make_compile_failed_execution_report(*build_source_exp->compile_failure_opt_)
            );
        return execute_result{std::move(process_submission_data_value)};
    }

    const auto execution_plan_exp = launch_planner_->make_execution_plan(*build_source_exp);
    if(!execution_plan_exp){
        return std::unexpected(judge_error{execution_plan_exp.error()});
    }

    auto testcase_snapshot_exp = testcase_snapshot_port_.acquire(
        queued_submission_value.problem_id
    );
    if(!testcase_snapshot_exp){
        return std::unexpected(testcase_snapshot_exp.error());
    }

    auto execution_report_exp = testcase_runner::run_all_testcases(
        *execution_plan_exp,
        *testcase_snapshot_exp
    );
    if(!execution_report_exp){
        return std::unexpected(execution_report_exp.error());
    }

    execution_result execution_result_value;
    execution_result_value.testcase_snapshot_value = std::move(*testcase_snapshot_exp);
    execution_result_value.execution_report_value = std::move(*execution_report_exp);
    return execute_result{std::move(execution_result_value)};
}
