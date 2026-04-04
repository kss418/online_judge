#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>

class judge_submission_port{
public:
    static std::expected<judge_submission_port, judge_error> create(
        const db_connection_config& db_config
    );

    judge_submission_port(judge_submission_port&& other) noexcept;
    judge_submission_port& operator=(judge_submission_port&& other) noexcept;
    ~judge_submission_port();

    judge_submission_port(const judge_submission_port&) = delete;
    judge_submission_port& operator=(const judge_submission_port&) = delete;

    std::expected<void, judge_error> mark_judging(std::int64_t submission_id);

    std::expected<void, judge_error> finalize_submission(
        const submission_dto::finalize_request& finalize_request_value
    );

    std::expected<void, judge_error> requeue_submission_immediately(
        std::int64_t submission_id,
        std::optional<std::string> reason_opt = std::nullopt
    );

private:
    explicit judge_submission_port(db_connection db_connection_value);

    db_connection db_connection_;
};
