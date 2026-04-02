#pragma once

#include "error/infra_error.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace auth_dto{
    struct token{
        std::string value;
    };

    struct hashed_token{
        std::string token_hash;
    };

    struct sign_up_request{
        std::string user_login_id;
        std::string raw_password;
    };

    struct credentials{
        std::string user_login_id;
        std::string raw_password;
    };

    struct hashed_sign_up_request{
        std::string user_login_id;
        std::string password_hash;
    };

    struct hashed_credentials{
        std::string user_login_id;
        std::string password_hash;
    };

    struct identity{
        std::int64_t user_id = 0;
        std::int32_t permission_level = 0;
        std::string user_login_id;
    };

    struct session{
        std::int64_t user_id = 0;
        std::int32_t permission_level = 0;
        std::string user_login_id;
        std::string token;
    };

    struct user_summary{
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::int32_t permission_level = 0;
        std::string created_at;
    };

    using user_summary_list = std::vector<user_summary>;

    bool is_valid(const token& token_value);
    bool is_valid(const hashed_token& hashed_token_value);
    bool is_valid(const sign_up_request& sign_up_request_value);
    bool is_valid(const credentials& credentials_value);
    bool is_valid(const hashed_sign_up_request& hashed_sign_up_request_value);
    bool is_valid(const hashed_credentials& hashed_credentials_value);
    std::expected<hashed_sign_up_request, infra_error> hash_sign_up_request(
        const sign_up_request& sign_up_request_value
    );

    std::expected<hashed_credentials, infra_error> hash_credentials(
        const credentials& credentials_value
    );
}
