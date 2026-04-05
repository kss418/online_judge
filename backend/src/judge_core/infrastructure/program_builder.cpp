#include "judge_core/infrastructure/program_builder.hpp"

#include "common/env_util.hpp"
#include "judge_core/infrastructure/cpp_runner.hpp"
#include "judge_core/infrastructure/java_runner.hpp"
#include "judge_core/infrastructure/python_runner.hpp"

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
}

std::expected<program_builder, judge_error> program_builder::create(){
    const auto cpp_compiler_path_exp = load_required_path_env(
        "JUDGE_CPP_COMPILER_PATH",
        "cpp compiler path is not configured"
    );
    if(!cpp_compiler_path_exp){
        return std::unexpected(cpp_compiler_path_exp.error());
    }

    const auto java_compiler_path_exp = load_required_path_env(
        "JUDGE_JAVA_COMPILER_PATH",
        "java compiler path is not configured"
    );
    if(!java_compiler_path_exp){
        return std::unexpected(java_compiler_path_exp.error());
    }

    return program_builder{
        std::move(*cpp_compiler_path_exp),
        std::move(*java_compiler_path_exp)
    };
}

program_builder::program_builder(
    std::filesystem::path cpp_compiler_path,
    std::filesystem::path java_compiler_path
) :
    cpp_compiler_path_(std::move(cpp_compiler_path)),
    java_compiler_path_(std::move(java_compiler_path))
{}

std::expected<program_build::build_artifact, sandbox_error> program_builder::build_source(
    const std::filesystem::path& source_file_path
){
    const auto extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        return cpp_runner::build(source_file_path, cpp_compiler_path_);
    }

    if(extension == ".py"){
        return python_runner::build(source_file_path);
    }

    if(extension == ".java"){
        return java_runner::build(source_file_path, java_compiler_path_);
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
