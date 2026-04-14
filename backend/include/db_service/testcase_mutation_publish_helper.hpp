#pragma once

#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace testcase_mutation_publish_helper{
    std::expected<problem_dto::version, service_error> increase_version_and_publish_current_snapshot(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_dto::version, service_error> get_current_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
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
