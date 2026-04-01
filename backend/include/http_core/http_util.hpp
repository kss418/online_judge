#pragma once

#include "dto/auth_dto.hpp"

#include <cstdint>

namespace http_util{
    bool is_owner_or_admin(
        const auth_dto::identity& auth_identity_value,
        std::int64_t owner_user_id
    );
}
