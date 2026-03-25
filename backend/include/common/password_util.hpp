#pragma once
#include "common/error_code.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace password_util{
    std::expected<std::string, error_code> get_password_hash(
        std::string_view password
    );
}
