#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace testcase_util{
    std::expected<problem_dto::testcase, error_code> create_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<problem_dto::testcase, error_code> get_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );

    std::expected<problem_dto::testcase_count, error_code> get_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, error_code> increase_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, error_code> decrease_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<std::vector<problem_dto::testcase>, error_code> list_testcases(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<void, error_code> delete_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> delete_all_testcases(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count, error_code> clear_testcase_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
