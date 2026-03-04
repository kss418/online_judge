#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <cstdint>
#include <expected>

struct limits{
    std::int32_t memory_limit_mb = 0;
    std::int32_t time_limit_ms = 0;
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
