#pragma once
#include "common/error_code.hpp"
#include "db_service/db_service_base.hpp"
#include "db_util/submission_util.hpp"

#include <chrono>
#include <expected>

class submission_service : public db_service_base<submission_service>{
public:
    std::expected<void, error_code> listen_submission_queue();
    std::expected<bool, error_code> wait_submission_notification(std::chrono::milliseconds timeout);

private:
    friend class db_service_base<submission_service>;

    explicit submission_service(db_connection connection);
};
