#include "judge_core/infrastructure/program_handler.hpp"

program_handler_registry::program_handler_registry(
    std::vector<std::unique_ptr<program_handler>> handlers
) :
    handlers_(std::move(handlers)){}

std::expected<const program_handler*, sandbox_error>
program_handler_registry::find_for_source_file(
    const std::filesystem::path& source_file_path
) const{
    for(const auto& handler_value : handlers_){
        if(handler_value->supports_source_file(source_file_path)){
            return handler_value.get();
        }
    }

    return std::unexpected(sandbox_error::invalid_argument);
}

std::expected<const program_handler*, sandbox_error>
program_handler_registry::find_for_build_artifact(
    const program_build::build_artifact& build_artifact_value
) const{
    for(const auto& handler_value : handlers_){
        if(handler_value->language() == build_artifact_value.language_){
            return handler_value.get();
        }
    }

    return std::unexpected(sandbox_error::invalid_argument);
}
