#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <expected>

class problem_service{
public:
    static std::expected<problem_service, error_code> create(db_connection db_connection);
    pqxx::connection& connection();
    const pqxx::connection& connection() const;

private:
    explicit problem_service(db_connection connection);
    db_connection db_connection_;
};
