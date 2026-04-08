#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>

class judge_submission_facade{
public:
    static std::expected<judge_submission_facade, judge_error> create(
        const db_connection_config& db_config
    );

    judge_submission_facade(judge_submission_facade&& other) noexcept;
    judge_submission_facade& operator=(judge_submission_facade&& other) noexcept;
    ~judge_submission_facade();

    judge_submission_facade(const judge_submission_facade&) = delete;
    judge_submission_facade& operator=(const judge_submission_facade&) = delete;

    std::expected<void, judge_error> mark_judging(
        const submission_dto::leased_submission& leased_submission_value
    );

    std::expected<void, judge_error> finalize_submission(
        const submission_dto::finalize_request& finalize_request_value
    );

    std::expected<void, judge_error> requeue_submission_immediately(
        const submission_dto::leased_submission& leased_submission_value,
        std::optional<std::string> reason_opt = std::nullopt
    );

private:
    explicit judge_submission_facade(db_connection db_connection_value);

    db_connection db_connection_;
};
