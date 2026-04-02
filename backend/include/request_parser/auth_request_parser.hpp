#pragma once

#include "dto/auth_dto.hpp"
#include "error/http_error.hpp"

#include <boost/json/fwd.hpp>

#include <expected>

namespace auth_request_parser{
    std::expected<auth_dto::sign_up_request, http_error> parse_sign_up_request(
        const boost::json::object& json
    );

    std::expected<auth_dto::credentials, http_error> parse_credentials(
        const boost::json::object& json
    );
}
