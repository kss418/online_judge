#pragma once

#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

namespace pqxx{
    class transaction_base;
}

namespace testcase_mutation_publish_helper{
    template <typename callback_type>
    using version_callback_result =
        std::invoke_result_t<callback_type, const problem_dto::version&>;

    std::expected<problem_dto::version, service_error> increase_version_and_publish_current_snapshot(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_dto::version, service_error> get_current_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    template <typename callback_type>
    auto with_published_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ) -> version_callback_result<callback_type>{
        const auto version_exp = increase_version_and_publish_current_snapshot(
            transaction,
            problem_reference_value
        );
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        return std::invoke(
            std::forward<callback_type>(callback),
            *version_exp
        );
    }
    template <typename callback_type>
    auto with_current_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        callback_type&& callback
    ) -> version_callback_result<callback_type>{
        const auto version_exp = get_current_version(
            transaction,
            problem_reference_value
        );
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        return std::invoke(
            std::forward<callback_type>(callback),
            *version_exp
        );
    }
    problem_dto::mutation_result make_mutation_result(
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value
    );
    problem_dto::testcase_mutation_result make_testcase_mutation_result(
        const problem_dto::testcase& testcase_value,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value
    );
    problem_dto::testcase_count_mutation_result make_testcase_count_mutation_result(
        const problem_dto::testcase_count& testcase_count_value,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::version& version_value
    );
}
