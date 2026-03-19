#include "db_event/submission_event_listener.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <utility>

std::expected<submission_event_listener, error_code> submission_event_listener::create(
    db_connection db_connection_value
){
    if(!db_connection_value.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return submission_event_listener(std::move(db_connection_value));
}

submission_event_listener::submission_event_listener(db_connection db_connection_value) :
    db_connection_(std::move(db_connection_value)){}

bool submission_event_listener::is_connected() const{
    return db_connection_.is_connected();
}

std::expected<void, error_code> submission_event_listener::listen_submission_queue(){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        db_connection_.connection().listen(
            submission_util::SUBMISSION_QUEUE_CHANNEL,
            [](const pqxx::notification&){}
        );
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> submission_event_listener::wait_submission_notification(
    std::chrono::milliseconds timeout
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(timeout < std::chrono::milliseconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        const auto timeout_seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        const auto remain = timeout - timeout_seconds;
        const auto timeout_microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(remain).count();

        const int notification_count = db_connection_.connection().await_notification(
            timeout_seconds.count(),
            static_cast<long>(timeout_microseconds)
        );
        return notification_count > 0;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
