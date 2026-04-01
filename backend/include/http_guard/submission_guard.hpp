#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "dto/submission_dto.hpp"
#include "http_core/http_response_util.hpp"

#include <cstdint>
#include <expected>

namespace submission_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<submission_dto::history_list, response_type> require_history(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::detail, response_type> require_detail(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, response_type> require_source_detail(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t submission_id
    );
    std::expected<void, response_type> require_source_access(
        const request_type& request,
        const auth_dto::identity& auth_identity_value,
        const submission_dto::source_detail& source_detail_value
    );
}
