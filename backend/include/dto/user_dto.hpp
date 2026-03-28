#pragma once

#include <cstdint>
#include <string>

namespace pqxx{
    class row;
}

namespace user_dto{
    struct summary{
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::string created_at;
    };

    summary make_summary_from_row(const pqxx::row& user_summary_row);
}
