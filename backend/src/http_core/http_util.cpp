#include "http_core/http_util.hpp"

#include "common/permission_util.hpp"

bool http_util::is_owner_or_admin(
    const auth_dto::identity& auth_identity_value,
    std::int64_t owner_user_id
){
    return permission_util::has_admin_access(auth_identity_value.permission_level) ||
        auth_identity_value.user_id == owner_user_id;
}
