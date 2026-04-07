#pragma once

#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <utility>
#include <variant>

class build_bundle{
public:
    static build_bundle make_runnable(runnable_program runnable_program_value){
        return build_bundle(std::move(runnable_program_value));
    }

    static build_bundle make_compile_failure(compile_failure compile_failure_value){
        return build_bundle(std::move(compile_failure_value));
    }

    bool success() const noexcept{
        return runnable_program_opt() != nullptr;
    }

    bool compile_failed() const noexcept{
        return compile_failure_opt() != nullptr;
    }

    const runnable_program* runnable_program_opt() const noexcept{
        return std::get_if<runnable_program>(&storage_);
    }

    const compile_failure* compile_failure_opt() const noexcept{
        return std::get_if<compile_failure>(&storage_);
    }

private:
    explicit build_bundle(runnable_program runnable_program_value) :
        storage_(std::move(runnable_program_value)){}

    explicit build_bundle(compile_failure compile_failure_value) :
        storage_(std::move(compile_failure_value)){}

    std::variant<runnable_program, compile_failure> storage_;
};
