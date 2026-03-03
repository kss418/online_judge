#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <string>

class problem_service{
public:
    static std::expected<problem_service, error_code> create(db_connection db_connection);
    pqxx::connection& connection();
    const pqxx::connection& connection() const;
    
    std::expected<std::int64_t, error_code> create_problem();
    std::expected<void, error_code> increase_problem_version(std::int64_t problem_id);
    std::expected<void, error_code> set_problem_limits(
        std::int64_t problem_id,
        std::int32_t memory_limit_mb,
        std::int32_t time_limit_ms
    );
    std::expected<void, error_code> increase_submission_count(std::int64_t problem_id);
    std::expected<void, error_code> increase_accepted_count(std::int64_t problem_id);
    
    std::expected<void, error_code> set_problem_statement(
        std::int64_t problem_id,
        const std::string& description,
        const std::string& input_format,
        const std::string& output_format,
        const std::string& note
    );
    
    std::expected<std::int64_t, error_code> create_problem_sample(
        std::int64_t problem_id,
        const std::string& sample_input,
        const std::string& sample_output
    );
private:
    explicit problem_service(db_connection connection);
    db_connection db_connection_;
};
