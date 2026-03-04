#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"
#include "dto/problem_dto.hpp"

#include <expected>
#include <string>
#include <vector>

class problem_core_service : public db_service_base<problem_core_service>{
public:
    std::expected<std::int64_t, error_code> create_problem();
    std::expected<limits, error_code> get_limits(std::int64_t problem_id);
    std::expected<void, error_code> set_limits(
        std::int64_t problem_id,
        const limits& limits_value
    );

    std::expected<std::int64_t, error_code> create_testcase(
        std::int64_t problem_id,
        const testcase& testcase_value
    );

    std::expected<testcase, error_code> get_testcase(
        std::int64_t problem_id,
        std::int32_t testcase_order
    );

    std::expected<std::vector<testcase>, error_code> list_testcases(std::int64_t problem_id);

    std::expected<void, error_code> set_testcase(
        std::int64_t problem_id,
        const testcase& testcase_value
    );

    std::expected<void, error_code> delete_testcase(std::int64_t problem_id);
private:
    std::expected<std::int32_t, error_code> increase_testcase_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> decrease_testcase_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    friend class db_service_base<problem_core_service>;

    explicit problem_core_service(db_connection connection);
};
