#include "judge_core/application/submission_builder.hpp"

#include "judge_core/infrastructure/program_build_types.hpp"

#include <utility>

namespace{
    execution_report::testcase_execution make_compile_execution(
        program_build::compile_run_result compile_run_result_value
    ){
        execution_report::testcase_execution compile_execution_value;
        compile_execution_value.exit_code = compile_run_result_value.exit_code_;
        compile_execution_value.termination_signal =
            compile_run_result_value.termination_signal_;
        compile_execution_value.killed_by_wall_clock =
            compile_run_result_value.killed_by_wall_clock_;
        compile_execution_value.stdout_text =
            std::move(compile_run_result_value.stdout_text_);
        compile_execution_value.stderr_text =
            std::move(compile_run_result_value.stderr_text_);
        compile_execution_value.stdout_bytes = static_cast<std::int64_t>(
            compile_run_result_value.stdout_bytes_
        );
        compile_execution_value.stderr_bytes = static_cast<std::int64_t>(
            compile_run_result_value.stderr_bytes_
        );
        compile_execution_value.max_rss_kb = static_cast<std::int64_t>(
            compile_run_result_value.max_rss_kb_
        );
        compile_execution_value.wall_time_ms = static_cast<std::int64_t>(
            compile_run_result_value.wall_time_ms_
        );
        compile_execution_value.cpu_time_ms = static_cast<std::int64_t>(
            compile_run_result_value.cpu_time_ms_
        );
        return compile_execution_value;
    }

    runnable_program_language to_runnable_program_language(
        program_build::source_language language_value
    ){
        switch(language_value){
        case program_build::source_language::cpp:
            return runnable_program_language::cpp;
        case program_build::source_language::python:
            return runnable_program_language::python;
        case program_build::source_language::java:
            return runnable_program_language::java;
        }

        return runnable_program_language::cpp;
    }

    build_bundle make_compile_failure_bundle(
        program_build::compile_failure compile_failure_value
    ){
        auto compile_execution_value = make_compile_execution(
            std::move(compile_failure_value.run_result_)
        );
        if(
            compile_failure_value.kind_ ==
            program_build::compile_failure_kind::compile_resource_exceeded
        ){
            return build_bundle::make_compile_resource_exceeded(
                compile_failure_value.resource_reason_,
                std::move(compile_execution_value)
            );
        }

        return build_bundle::make_user_compile_error(
            std::move(compile_execution_value)
        );
    }

    runnable_program make_runnable_program(
        program_build::build_artifact build_artifact_value
    ){
        runnable_program runnable_program_value;
        runnable_program_value.language =
            to_runnable_program_language(build_artifact_value.language_);
        runnable_program_value.workspace_host_path =
            std::move(build_artifact_value.workspace_host_path_);
        runnable_program_value.entry_file_host_path =
            std::move(build_artifact_value.entry_file_host_path_);
        runnable_program_value.main_class_name =
            std::move(build_artifact_value.main_class_name_);
        return runnable_program_value;
    }
}

std::expected<submission_builder, judge_error> submission_builder::create(){
    auto build_dispatcher_exp = build_dispatcher::create();
    if(!build_dispatcher_exp){
        return std::unexpected(build_dispatcher_exp.error());
    }

    return submission_builder(std::move(*build_dispatcher_exp));
}

submission_builder::submission_builder(
    build_dispatcher build_dispatcher_value
) :
    build_dispatcher_(std::move(build_dispatcher_value)){}

submission_builder::submission_builder(
    submission_builder&& other
) noexcept = default;

submission_builder& submission_builder::operator=(
    submission_builder&& other
) noexcept = default;

submission_builder::~submission_builder() = default;

build_bundle submission_builder::build(
    const std::filesystem::path& source_file_path
){
    auto build_source_exp = build_dispatcher_.build_source(source_file_path);
    if(!build_source_exp){
        return build_bundle::make_infra_failure(
            judge_error{build_source_exp.error()}
        );
    }

    auto build_artifact_value = std::move(*build_source_exp);
    if(build_artifact_value.compile_failure_opt_.has_value()){
        return make_compile_failure_bundle(
            std::move(*build_artifact_value.compile_failure_opt_)
        );
    }

    return build_bundle::make_success(
        make_runnable_program(std::move(build_artifact_value))
    );
}
