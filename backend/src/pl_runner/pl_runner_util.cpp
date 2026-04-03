#include "pl_runner/pl_runner_util.hpp"

#include "pl_runner/cpp_runner.hpp"
#include "pl_runner/java_runner.hpp"
#include "pl_runner/python_runner.hpp"

#include <cstdlib>
#include <utility>

pl_runner_util& pl_runner_util::instance(){
    static pl_runner_util pl_runner_util_value;
    pl_runner_util_value.initialize_if_needed();
    return pl_runner_util_value;
}

void pl_runner_util::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(
        cpp_compiler_path_.has_value() &&
        python_path_.has_value() &&
        java_compiler_path_.has_value() &&
        java_runtime_path_.has_value()
    ){
        return;
    }

    const char* cpp_compiler_path = std::getenv("JUDGE_CPP_COMPILER_PATH");
    if(cpp_compiler_path != nullptr && *cpp_compiler_path != '\0'){
        cpp_compiler_path_ = std::filesystem::path(cpp_compiler_path);
    }

    const char* python_path = std::getenv("JUDGE_PYTHON_PATH");
    if(python_path != nullptr && *python_path != '\0'){
        python_path_ = std::filesystem::path(python_path);
    }

    const char* java_compiler_path = std::getenv("JUDGE_JAVA_COMPILER_PATH");
    if(java_compiler_path != nullptr && *java_compiler_path != '\0'){
        java_compiler_path_ = std::filesystem::path(java_compiler_path);
    }

    const char* java_runtime_path = std::getenv("JUDGE_JAVA_RUNTIME_PATH");
    if(java_runtime_path != nullptr && *java_runtime_path != '\0'){
        java_runtime_path_ = std::filesystem::path(java_runtime_path);
    }
}

pl_runner_util::prepared_source pl_runner_util::make_compile_failed_prepared_source(
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

std::expected<pl_runner_util::prepared_source, sandbox_error> pl_runner_util::prepare_source(
    const std::filesystem::path& source_file_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        if(!cpp_compiler_path_.has_value() || cpp_compiler_path_->empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        return cpp_runner::prepare(source_file_path, *cpp_compiler_path_);
    }

    if(extension == ".py"){
        if(!python_path_.has_value() || python_path_->empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        return python_runner::prepare(source_file_path, *python_path_);
    }

    if(extension == ".java"){
        if(
            !java_compiler_path_.has_value() ||
            java_compiler_path_->empty() ||
            !java_runtime_path_.has_value() ||
            java_runtime_path_->empty()
        ){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        return java_runner::prepare(
            source_file_path,
            *java_compiler_path_,
            *java_runtime_path_
        );
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
