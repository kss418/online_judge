#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>

enum class submission_status{
    queued,
    judging,
    accepted,
    wrong_answer,
    time_limit_exceeded,
    memory_limit_exceeded,
    runtime_error,
    compile_error,
    output_exceeded
};

std::string to_string(submission_status status);

struct submission_create_request{
    std::int64_t user_id;
    std::int64_t problem_id;
    std::string language;
    std::string source_code;
};

class submission_service{
public:
    static std::expected<submission_service, error_code> create();
    pqxx::connection& connection();
    const pqxx::connection& connection() const;

    std::expected<std::int64_t, error_code> create_submission(const submission_create_request& request);
    std::expected<void, error_code> update_submission_status(
        std::int64_t submission_id,
        submission_status to_status,
        const std::optional<std::string>& reason = std::nullopt
    );

private:
    explicit submission_service(db_connection connection);
    db_connection db_connection_;
};
