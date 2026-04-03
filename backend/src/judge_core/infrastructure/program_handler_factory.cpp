#include "judge_core/infrastructure/program_handler_factory.hpp"

#include "judge_core/infrastructure/cpp_runner.hpp"
#include "judge_core/infrastructure/java_runner.hpp"
#include "judge_core/infrastructure/program_handler.hpp"
#include "judge_core/infrastructure/python_runner.hpp"

std::shared_ptr<const program_handler_registry> make_program_handler_registry(
    const toolchain_config& toolchain_config_value
){
    std::vector<std::unique_ptr<program_handler>> handlers;
    handlers.push_back(
        make_cpp_program_handler(toolchain_config_value.cpp_compiler_path)
    );
    handlers.push_back(
        make_python_program_handler(toolchain_config_value.python_path)
    );
    handlers.push_back(
        make_java_program_handler(
            toolchain_config_value.java_compiler_path,
            toolchain_config_value.java_runtime_path
        )
    );
    return std::make_shared<program_handler_registry>(std::move(handlers));
}
