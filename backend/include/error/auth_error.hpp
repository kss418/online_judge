#pragma once

#include "error/http_error.hpp"
#include "error/service_error.hpp"

#include <optional>

namespace auth_error{
    http_error missing_or_invalid_bearer_token();
    http_error invalid_or_expired_token();
    http_error admin_bearer_token_required();
    http_error superadmin_bearer_token_required();
    http_error invalid_credentials();

    std::optional<http_error> map_login_service_error(
        const service_error& error
    );
    std::optional<http_error> map_token_service_error(
        const service_error& error
    );
}
