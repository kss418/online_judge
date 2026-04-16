#pragma once

#include "dto/problem_dto.hpp"
#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"

#include <cstdint>
#include <utility>

namespace http_handler_spec{
    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_testcase_ref_json_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            problem_dto::testcase_ref{
                .problem_id = problem_id,
                .testcase_order = testcase_order
            },
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_testcase_ref_json_spec(
        std::int64_t problem_id,
        std::int32_t testcase_order,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            problem_dto::testcase_ref{
                .problem_id = problem_id,
                .testcase_order = testcase_order
            },
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_admin_problem_json_spec(
        std::int64_t problem_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            problem_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_dto::reference{problem_id}),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_admin_problem_json_spec(
        std::int64_t problem_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_json_spec(
            problem_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_dto::reference{problem_id}),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename... guard_types
    >
    auto make_admin_problem_message_spec(
        std::int64_t problem_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        guard_types&&... guards
    ){
        return make_single_path_value_message_spec(
            problem_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_dto::reference{problem_id}),
            std::forward<guard_types>(guards)...
        );
    }

    template <
        typename builder_type,
        typename execute_type,
        typename serialize_type,
        typename error_response_factory_type,
        typename... guard_types
    >
    auto make_admin_problem_message_spec(
        std::int64_t problem_id,
        builder_type&& build_command,
        execute_type&& execute,
        serialize_type&& serialize,
        http_endpoint::spec_options<error_response_factory_type> options,
        guard_types&&... guards
    ){
        return make_single_path_value_message_spec(
            problem_id,
            std::forward<builder_type>(build_command),
            std::forward<execute_type>(execute),
            std::forward<serialize_type>(serialize),
            std::move(options),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_dto::reference{problem_id}),
            std::forward<guard_types>(guards)...
        );
    }
}
