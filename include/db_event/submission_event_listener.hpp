#pragma once
#include "common/db_connection.hpp"
#include "common/error_code.hpp"

#include <chrono>
#include <expected>

class submission_event_listener{
public:
    static std::expected<submission_event_listener, error_code> create(
        db_connection db_connection_value
    );

    std::expected<void, error_code> listen_submission_queue();
    std::expected<bool, error_code> wait_submission_notification(std::chrono::milliseconds timeout);

private:
    bool is_connected() const;

    explicit submission_event_listener(db_connection db_connection_value);

    db_connection db_connection_;
};
