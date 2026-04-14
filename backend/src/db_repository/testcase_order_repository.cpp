#include "db_repository/testcase_order_repository.hpp"

#include "db_repository/testcase_query_repository.hpp"

#include <pqxx/pqxx>

#include <algorithm>

std::expected<void, repository_error> testcase_order_repository::move_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    std::int32_t target_testcase_order
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t source_testcase_order = testcase_reference_value.testcase_order;
    if(!problem_dto::is_valid(testcase_reference_value) || target_testcase_order <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    if(source_testcase_order == target_testcase_order){
        const auto testcase_exp = testcase_query_repository::get_testcase(
            transaction,
            testcase_reference_value
        );
        if(!testcase_exp){
            return std::unexpected(testcase_exp.error());
        }

        return {};
    }

    const auto testcase_exp = testcase_query_repository::get_testcase(
        transaction,
        testcase_reference_value
    );
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    transaction.exec(
        "SET CONSTRAINTS problem_testcases_problem_id_testcase_order_unique DEFERRED"
    );

    const std::int32_t min_testcase_order = std::min(
        source_testcase_order,
        target_testcase_order
    );
    const std::int32_t max_testcase_order = std::max(
        source_testcase_order,
        target_testcase_order
    );

    const auto move_result = transaction.exec(
        "UPDATE problem_testcases "
        "SET testcase_order = CASE "
        "WHEN testcase_order = $2 THEN $3 "
        "WHEN $2 < $3 AND testcase_order > $2 AND testcase_order <= $3 "
        "THEN testcase_order - 1 "
        "WHEN $2 > $3 AND testcase_order >= $3 AND testcase_order < $2 "
        "THEN testcase_order + 1 "
        "ELSE testcase_order "
        "END "
        "WHERE "
        "problem_id = $1 AND "
        "testcase_order BETWEEN $4 AND $5",
        pqxx::params{
            problem_id,
            source_testcase_order,
            target_testcase_order,
            min_testcase_order,
            max_testcase_order
        }
    );

    const auto expected_affected_rows = static_cast<decltype(move_result.affected_rows())>(
        max_testcase_order - min_testcase_order + 1
    );
    if(move_result.affected_rows() != expected_affected_rows){
        return std::unexpected(repository_error::conflict);
    }

    return {};
}
