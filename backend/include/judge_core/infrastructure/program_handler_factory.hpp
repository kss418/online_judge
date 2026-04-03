#pragma once

#include "judge_core/infrastructure/toolchain_config.hpp"

#include <memory>

class program_handler_registry;

std::shared_ptr<const program_handler_registry> make_program_handler_registry(
    const toolchain_config& toolchain_config_value
);
