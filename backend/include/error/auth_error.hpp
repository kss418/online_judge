#pragma once

#include "error/http_error.hpp"

namespace auth_error{
    http_error missing_or_invalid_bearer_token();
    http_error invalid_or_expired_token();
    http_error admin_bearer_token_required();
    http_error superadmin_bearer_token_required();
    http_error invalid_credentials();
}
