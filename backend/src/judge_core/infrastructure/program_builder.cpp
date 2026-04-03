#include "judge_core/infrastructure/program_builder.hpp"

#include "judge_core/infrastructure/program_handler.hpp"

program_builder::program_builder(
    std::shared_ptr<const program_handler_registry> handler_registry
) :
    handler_registry_(std::move(handler_registry))
{}

std::expected<program_build::build_artifact, sandbox_error> program_builder::build_source(
    const std::filesystem::path& source_file_path
){
    if(handler_registry_ == nullptr){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto handler_exp = handler_registry_->find_for_source_file(source_file_path);
    if(!handler_exp){
        return std::unexpected(handler_exp.error());
    }

    return (*handler_exp)->build_source(source_file_path);
}
