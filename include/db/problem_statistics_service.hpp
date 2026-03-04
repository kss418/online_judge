#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <cstdint>
#include <expected>

class problem_statistics_service : public db_service_base<problem_statistics_service>{
public:
    std::expected<void, error_code> increase_submission_count(std::int64_t problem_id);
    std::expected<void, error_code> increase_accepted_count(std::int64_t problem_id);

private:
    friend class db_service_base<problem_statistics_service>;

    explicit problem_statistics_service(db_connection connection);
};
