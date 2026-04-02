#pragma once
#include "error/infra_error.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace password_util{
    std::expected<std::string, infra_error> get_password_hash(
        std::string_view password
    );
}
