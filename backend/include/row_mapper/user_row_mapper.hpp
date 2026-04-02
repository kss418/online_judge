#pragma once

#include "dto/user_dto.hpp"

namespace pqxx{
    class result;
    class row;
}

namespace user_row_mapper{
    user_dto::summary map_summary_row(const pqxx::row& user_summary_row);
    user_dto::list_item map_list_item_row(const pqxx::row& user_list_row);
    user_dto::list map_list_result(const pqxx::result& user_list_result);
}
