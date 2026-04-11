#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace create_submission_action{
    using command = submission_internal_dto::create_submission_command;

    struct error{
        enum class kind{
            service_error,
            submission_banned
        };

        kind kind_value = kind::service_error;
        service_error service_error_value{service_error_code::internal};

        static error from_service_error(const service_error& service_error_value);
        static error submission_banned();

        bool is_submission_banned() const;
    };

    std::expected<submission_response_dto::queued_response, error> execute(
        db_connection& connection,
        const command& command_value
    );
}
