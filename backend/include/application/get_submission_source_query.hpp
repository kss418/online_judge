#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace get_submission_source_query{
    struct command{
        std::int64_t submission_id = 0;
        auth_dto::identity viewer_identity;
    };

    struct error{
        enum class kind{
            service_error,
            source_access_denied
        };

        kind kind_value = kind::service_error;
        service_error service_error_value{service_error_code::internal};

        static error from_service_error(const service_error& service_error_value);
        static error source_access_denied();

        bool is_source_access_denied() const;
    };

    std::expected<submission_response_dto::source_detail, error> execute(
        db_connection& connection,
        const command& command_value
    );
}
