#include "judge_core/infrastructure/program_builder.hpp"

#include "judge_core/infrastructure/cpp_runner.hpp"
#include "judge_core/infrastructure/java_runner.hpp"
#include "judge_core/infrastructure/python_runner.hpp"

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
