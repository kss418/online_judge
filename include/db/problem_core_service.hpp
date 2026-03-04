#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <cstdint>
#include <expected>
#include <string>

struct limits{
    std::int32_t memory_limit_mb = 0;
    std::int32_t time_limit_ms = 0;
};

struct testcase{
    std::int64_t testcase_id = 0;
    std::int32_t testcase_order = 0;
    std::string testcase_input;
    std::string testcase_output;
};

class problem_core_service : public db_service_base<problem_core_service>{
public:
    std::expected<std::int64_t, error_code> create_problem();
    std::expected<limits, error_code> get_limits(std::int64_t problem_id);
    std::expected<void, error_code> set_limits(
        std::int64_t problem_id,
        std::int32_t memory_limit_mb,
        std::int32_t time_limit_ms
    );

    std::expected<std::int64_t, error_code> create_testcase(
        std::int64_t problem_id,
        const std::string& testcase_input,
        const std::string& testcase_output
    );

    std::expected<testcase, error_code> get_testcase(
        std::int64_t problem_id,
        std::int32_t testcase_order
    );

    std::expected<void, error_code> set_testcase(
        std::int64_t problem_id,
        std::int32_t testcase_order,
        const std::string& testcase_input,
        const std::string& testcase_output
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
