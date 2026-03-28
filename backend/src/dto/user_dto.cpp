#include "dto/user_dto.hpp"

#include <pqxx/pqxx>

user_dto::summary user_dto::make_summary_from_row(const pqxx::row& user_summary_row){
    user_dto::summary summary_value;
    summary_value.user_id = user_summary_row[0].as<std::int64_t>();
    summary_value.user_name = user_summary_row[1].as<std::string>();
    summary_value.created_at = user_summary_row[2].as<std::string>();
    return summary_value;
}
