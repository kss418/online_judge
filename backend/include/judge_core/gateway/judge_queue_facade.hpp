#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/infrastructure/submission_queue_source.hpp"

#include <chrono>
#include <expected>
#include <optional>

class judge_queue_facade{
public:
    static std::expected<judge_queue_facade, judge_error> create(
        const db_connection_config& db_config
    );

    judge_queue_facade(judge_queue_facade&& other) noexcept;
    judge_queue_facade& operator=(judge_queue_facade&& other) noexcept;
    ~judge_queue_facade();

    judge_queue_facade(const judge_queue_facade&) = delete;
    judge_queue_facade& operator=(const judge_queue_facade&) = delete;

    std::expected<std::optional<submission_domain_dto::leased_submission>, judge_error>
    try_lease_next(std::chrono::seconds lease_duration);

    std::expected<void, judge_error> wait_for_work(
        std::chrono::milliseconds notification_wait_timeout
    );

private:
    judge_queue_facade(
        db_connection lease_db_connection,
        submission_queue_source submission_queue_source_value
    );

    db_connection lease_db_connection_;
    submission_queue_source submission_queue_source_;
};
