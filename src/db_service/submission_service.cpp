#include "db_service/submission_service.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <utility>

submission_service::submission_service(db_connection connection) :
    db_service_base<submission_service>(std::move(connection)){}

std::expected<void, error_code> submission_service::listen_submission_queue(){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        connection().listen(
            submission_util::SUBMISSION_QUEUE_CHANNEL,
            [](const pqxx::notification&){}
        );
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<bool, error_code> submission_service::wait_submission_notification(
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

        const int notification_count = connection().await_notification(
            timeout_seconds.count(),
            static_cast<long>(timeout_microseconds)
        );
        return notification_count > 0;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
