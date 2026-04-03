#pragma once

#include "common/db_connection.hpp"
#include "error/submission_event_error.hpp"

#include <chrono>
#include <expected>
#include <memory>

class submission_event_listener;

class submission_queue_source{
public:
    static std::expected<submission_queue_source, submission_event_error> create(
        db_connection db_connection_value
    );

    submission_queue_source(submission_queue_source&&) noexcept;
    submission_queue_source& operator=(submission_queue_source&&) noexcept;
    ~submission_queue_source();

    std::expected<bool, submission_event_error> wait_submission_notification(
        std::chrono::milliseconds timeout
    );

private:
    explicit submission_queue_source(
        std::unique_ptr<submission_event_listener> submission_event_listener_value
    );

    std::unique_ptr<submission_event_listener> submission_event_listener_;
};
