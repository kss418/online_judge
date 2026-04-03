#pragma once
#include "common/db_connection.hpp"
#include "error/submission_event_error.hpp"

#include <chrono>
#include <expected>

class submission_event_listener{
public:
    static std::expected<submission_event_listener, submission_event_error> create(
        db_connection db_connection_value
    );

    std::expected<void, submission_event_error> listen_submission_queue();
    std::expected<bool, submission_event_error> wait_submission_notification(
        std::chrono::milliseconds timeout
    );

private:
    bool is_connected() const;

    explicit submission_event_listener(db_connection db_connection_value);

    db_connection db_connection_;
};
