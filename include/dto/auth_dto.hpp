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

    struct sign_up_request{
        std::string user_name;
        std::string user_login_id;
        std::string raw_password;
    };

    struct credentials{
        std::string user_login_id;
        std::string raw_password;
    };

    struct hashed_sign_up_request{
        std::string user_name;
        std::string user_login_id;
        std::string password_hash;
    };

    struct hashed_credentials{
        std::string user_login_id;
        std::string password_hash;
    };

    struct identity{
        std::int64_t user_id = 0;
        bool is_admin = false;
        std::string user_name;
    };

    struct session{
        std::int64_t user_id = 0;
        bool is_admin = false;
        std::string user_name;
        std::string token;
    };

    std::expected<sign_up_request, dto_validation_error> make_sign_up_request_from_json(
        const boost::json::object& json
    );

    std::expected<credentials, dto_validation_error> make_credentials_from_json(
        const boost::json::object& json
    );

    std::expected<hashed_sign_up_request, error_code> hash_sign_up_request(
        const sign_up_request& sign_up_request_value
    );

    std::expected<hashed_credentials, error_code> hash_credentials(
        const credentials& credentials_value
    );
}
