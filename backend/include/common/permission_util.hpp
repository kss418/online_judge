#pragma once

#include <cstdint>
#include <string_view>

namespace permission_util{
    inline constexpr std::int32_t USER = 0;
    inline constexpr std::int32_t ADMIN = 1;
    inline constexpr std::int32_t SUPERADMIN = 2;

    inline constexpr bool is_valid_permission_level(std::int32_t permission_level){
        return permission_level >= USER && permission_level <= SUPERADMIN;
    }

    inline constexpr bool has_admin_access(std::int32_t permission_level){
        return permission_level >= ADMIN;
    }

    inline constexpr bool has_superadmin_access(std::int32_t permission_level){
        return permission_level >= SUPERADMIN;
    }

    inline constexpr std::string_view role_name(std::int32_t permission_level){
        if(has_superadmin_access(permission_level)){
            return "superadmin";
        }

        if(has_admin_access(permission_level)){
            return "admin";
        }

        return "user";
    }
}
