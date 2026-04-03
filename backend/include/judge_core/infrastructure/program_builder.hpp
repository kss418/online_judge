#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <memory>

class program_handler_registry;

class program_builder{
public:
    explicit program_builder(
        std::shared_ptr<const program_handler_registry> handler_registry
    );

    std::expected<program_build::build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    );

private:
    std::shared_ptr<const program_handler_registry> handler_registry_;
};
