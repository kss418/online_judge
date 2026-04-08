#pragma once

#include "error/judge_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

class build_bundle{
public:
    struct build_success{
        runnable_program program;
    };

    struct user_compile_error{
        execution_report::testcase_execution compile_execution;
    };

    struct compile_resource_exceeded{
        program_build::compile_resource_exceeded_reason reason =
            program_build::compile_resource_exceeded_reason::unknown;
        execution_report::testcase_execution compile_execution;
    };

    struct build_infra_failure{
        judge_error error = judge_error::internal;
    };

    static build_bundle make_success(runnable_program runnable_program_value){
        build_success success_value;
        success_value.program = std::move(runnable_program_value);
        return build_bundle(std::move(success_value));
    }

    static build_bundle make_user_compile_error(
        execution_report::testcase_execution compile_execution_value
    ){
        user_compile_error error_value;
        error_value.compile_execution = std::move(compile_execution_value);
        return build_bundle(std::move(error_value));
    }

    static build_bundle make_compile_resource_exceeded(
        program_build::compile_resource_exceeded_reason reason,
        execution_report::testcase_execution compile_execution_value
    ){
        compile_resource_exceeded exceeded_value;
        exceeded_value.reason = reason;
        exceeded_value.compile_execution = std::move(compile_execution_value);
        return build_bundle(std::move(exceeded_value));
    }

    static build_bundle make_infra_failure(judge_error error_value){
        build_infra_failure failure_value;
        failure_value.error = std::move(error_value);
        return build_bundle(std::move(failure_value));
    }

    bool success() const noexcept{
        return std::holds_alternative<build_success>(storage_);
    }

    bool compile_failed() const noexcept{
        return is_user_compile_error() || is_compile_resource_exceeded();
    }

    bool is_user_compile_error() const noexcept{
        return std::holds_alternative<user_compile_error>(storage_);
    }

    bool is_compile_resource_exceeded() const noexcept{
        return std::holds_alternative<compile_resource_exceeded>(storage_);
    }

    bool infra_failed() const noexcept{
        return std::holds_alternative<build_infra_failure>(storage_);
    }

    const runnable_program& artifact() const{
        return std::get<build_success>(storage_).program;
    }

    const user_compile_error& user_compile_error_value() const{
        return std::get<user_compile_error>(storage_);
    }

    const compile_resource_exceeded& compile_resource_exceeded_value() const{
        return std::get<compile_resource_exceeded>(storage_);
    }

    const build_infra_failure& infra_failure() const{
        return std::get<build_infra_failure>(storage_);
    }

private:
    explicit build_bundle(build_success success_value) :
        storage_(std::move(success_value)){}

    explicit build_bundle(user_compile_error error_value) :
        storage_(std::move(error_value)){}

    explicit build_bundle(compile_resource_exceeded exceeded_value) :
        storage_(std::move(exceeded_value)){}

    explicit build_bundle(build_infra_failure failure_value) :
        storage_(std::move(failure_value)){}

    std::variant<
        build_success,
        user_compile_error,
        compile_resource_exceeded,
        build_infra_failure
    > storage_;
};
