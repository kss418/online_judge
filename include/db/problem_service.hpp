#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>

struct problem_create_response{
    std::int64_t problem_id = 0;
    std::int32_t version = 1;
};

class problem_service{
public:
    static std::expected<problem_service, error_code> create(db_connection db_connection);
    pqxx::connection& connection();
    const pqxx::connection& connection() const;
    std::expected<problem_create_response, error_code> create_problem();
    std::expected<void, error_code> increase_problem_version(std::int64_t problem_id);

private:
    explicit problem_service(db_connection connection);
    db_connection db_connection_;
};
