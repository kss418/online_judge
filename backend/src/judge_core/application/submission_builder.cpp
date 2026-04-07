#include "judge_core/application/submission_builder.hpp"

#include "judge_core/infrastructure/program_build_types.hpp"

#include <utility>

namespace{
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

    compile_failure make_compile_failure(
        program_build::compile_failure compile_failure_value
    ){
        compile_failure app_compile_failure_value;
        app_compile_failure_value.exit_code = compile_failure_value.exit_code_;
        app_compile_failure_value.stderr_text =
            std::move(compile_failure_value.stderr_text_);
        return app_compile_failure_value;
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

std::expected<submission_builder::build_result, judge_error> submission_builder::build(
    const std::filesystem::path& source_file_path
){
    auto build_source_exp = build_dispatcher_.build_source(source_file_path);
    if(!build_source_exp){
        return std::unexpected(judge_error{build_source_exp.error()});
    }

    auto build_artifact_value = std::move(*build_source_exp);
    if(!build_artifact_value.is_runnable()){
        return build_result{
            make_compile_failure(std::move(*build_artifact_value.compile_failure_opt_))
        };
    }

    return build_result{make_runnable_program(std::move(build_artifact_value))};
}
