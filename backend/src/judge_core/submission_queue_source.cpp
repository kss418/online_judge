#include "judge_core/submission_queue_source.hpp"

#include "db_event/submission_event_listener.hpp"

#include <memory>
#include <utility>

std::expected<submission_queue_source, submission_event_error> submission_queue_source::create(
    db_connection db_connection_value
){
    auto submission_event_listener_exp = submission_event_listener::create(
        std::move(db_connection_value)
    );
    if(!submission_event_listener_exp){
        return std::unexpected(submission_event_listener_exp.error());
    }

    const auto listen_submission_queue_exp =
        submission_event_listener_exp->listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    return submission_queue_source(
        std::make_unique<submission_event_listener>(
            std::move(*submission_event_listener_exp)
        )
    );
}

submission_queue_source::submission_queue_source(
    std::unique_ptr<submission_event_listener> submission_event_listener_value
) :
    submission_event_listener_(std::move(submission_event_listener_value)){}

submission_queue_source::submission_queue_source(
    submission_queue_source&&
) noexcept = default;

submission_queue_source& submission_queue_source::operator=(
    submission_queue_source&&
) noexcept = default;

submission_queue_source::~submission_queue_source() = default;

std::expected<bool, submission_event_error> submission_queue_source::wait_submission_notification(
    std::chrono::milliseconds timeout
){
    return submission_event_listener_->wait_submission_notification(timeout);
}
