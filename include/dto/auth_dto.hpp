#pragma once

#include <boost/json/fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace auth_dto{
    struct credentials{
        std::string user_login_id;
        std::string raw_password;
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

    std::optional<credentials> make_credentials(
        const boost::json::object& request_object
    );
}
