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

    inline constexpr bool is_admin(std::int32_t permission_level){
        return permission_level >= ADMIN;
    }

    inline constexpr bool is_superadmin(std::int32_t permission_level){
        return permission_level >= SUPERADMIN;
    }

    inline constexpr std::string_view role_name(std::int32_t permission_level){
        if(is_superadmin(permission_level)){
            return "superadmin";
        }

        if(is_admin(permission_level)){
            return "admin";
        }

        return "user";
    }
}
