#pragma once

#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <expected>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace testcase_query_repository{
    std::expected<problem_dto::testcase, repository_error> get_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    );
    std::expected<problem_dto::testcase_count, repository_error> get_testcase_count(
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
}
