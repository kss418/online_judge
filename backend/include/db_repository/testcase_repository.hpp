#pragma once

#include "common/repository_error.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace testcase_repository{
    std::expected<problem_dto::testcase, repository_error> create_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<problem_dto::testcase, repository_error> get_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );

    std::expected<problem_dto::testcase_count, repository_error> get_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, repository_error> increase_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, repository_error> decrease_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<std::vector<problem_dto::testcase>, repository_error> list_testcases(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::vector<problem_dto::testcase_summary>, repository_error> list_testcase_summaries(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> set_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<void, repository_error> move_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        std::int32_t target_testcase_order
    );

    std::expected<void, repository_error> delete_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );

    std::expected<void, repository_error> decrease_order(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );

    std::expected<void, repository_error> delete_testcase_and_shift_after(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );

    std::expected<void, repository_error> delete_all_testcases(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, repository_error> clear_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
