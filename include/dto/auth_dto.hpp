#pragma once

#include "common/error_code.hpp"
#include "dto/dto_validation_error.hpp"

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>

namespace auth_dto{
    struct token{
        std::string value;
    };

    struct hashed_token{
        std::string token_hash;
    };

    struct credentials{
        std::string user_login_id;
        std::string raw_password;
    };

    struct hashed_credentials{
        std::string user_login_id;
        std::string password_hash;
    };

    struct identity{
        std::int64_t user_id = 0;
        bool is_admin = false;
    };

    struct session{
        std::int64_t user_id = 0;
        bool is_admin = false;
        std::string token;
    };

    std::expected<credentials, dto_validation_error> make_credentials_from_json(
        const boost::json::object& json
    );

    std::expected<hashed_credentials, error_code> hash_credentials(
        const credentials& credentials_value
    );
}
