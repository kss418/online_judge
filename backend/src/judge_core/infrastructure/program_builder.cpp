#include "judge_core/infrastructure/program_builder.hpp"

#include "judge_core/infrastructure/cpp_runner.hpp"
#include "judge_core/infrastructure/java_runner.hpp"
#include "judge_core/infrastructure/python_runner.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <cstdlib>
#include <utility>

namespace{
    constexpr std::chrono::milliseconds COMPILE_TIME_LIMIT{30000};
    constexpr std::int64_t COMPILE_MEMORY_LIMIT_MB = 1024;

    sandbox_runner::run_options make_compile_run_options(
        const std::filesystem::path& workspace_host_path,
        sandbox_runner::mount_profile mount_profile =
            sandbox_runner::mount_profile::default_profile
    ){
        sandbox_runner::run_options run_options_value;
        run_options_value.workspace_host_path = workspace_host_path;
        run_options_value.time_limit = COMPILE_TIME_LIMIT;
        run_options_value.memory_limit_mb = COMPILE_MEMORY_LIMIT_MB;
        run_options_value.policy = sandbox_runner::policy_profile::compile;
        run_options_value.mounts = mount_profile;
        return run_options_value;
    }
}

program_builder& program_builder::instance(){
    static program_builder program_builder_value;
    program_builder_value.initialize_if_needed();
    return program_builder_value;
}

void program_builder::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(
        cpp_compiler_path_.has_value() &&
        java_compiler_path_.has_value()
    ){
        return;
    }

    const char* cpp_compiler_path = std::getenv("JUDGE_CPP_COMPILER_PATH");
    if(cpp_compiler_path != nullptr && *cpp_compiler_path != '\0'){
        cpp_compiler_path_ = std::filesystem::path(cpp_compiler_path);
    }

    const char* java_compiler_path = std::getenv("JUDGE_JAVA_COMPILER_PATH");
    if(java_compiler_path != nullptr && *java_compiler_path != '\0'){
        java_compiler_path_ = std::filesystem::path(java_compiler_path);
    }
}

std::expected<program_build::build_artifact, sandbox_error> program_builder::build_source(
    const std::filesystem::path& source_file_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        if(!cpp_compiler_path_.has_value() || cpp_compiler_path_->empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        auto runner_build_exp = cpp_runner::build(
            source_file_path,
            *cpp_compiler_path_,
            make_compile_run_options(source_file_path.parent_path())
        );
        if(!runner_build_exp){
            return std::unexpected(runner_build_exp.error());
        }

        return std::move(*runner_build_exp);
    }

    if(extension == ".py"){
        auto runner_build_exp = python_runner::build(source_file_path);
        if(!runner_build_exp){
            return std::unexpected(runner_build_exp.error());
        }

        return std::move(*runner_build_exp);
    }

    if(extension == ".java"){
        if(
            !java_compiler_path_.has_value() ||
            java_compiler_path_->empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        auto runner_build_exp = java_runner::build(
            source_file_path,
            *java_compiler_path_,
            make_compile_run_options(
                source_file_path.parent_path(),
                sandbox_runner::mount_profile::java_profile
            )
        );
        if(!runner_build_exp){
            return std::unexpected(runner_build_exp.error());
        }

        return std::move(*runner_build_exp);
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
