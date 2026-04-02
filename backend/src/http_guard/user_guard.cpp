#include "http_guard/user_guard.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::summary, user_guard::response_type> user_guard::require_summary(
    const request_type& request,
    db_connection& db_connection,
    std::int64_t user_id
){
    const auto user_summary_exp = user_service::get_summary(
        db_connection,
        user_id
    );
    if(!user_summary_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(request, user_summary_exp.error()));
    }

    return std::move(*user_summary_exp);
}

std::expected<user_dto::summary, user_guard::response_type> user_guard::require_summary_by_login_id(
    const request_type& request,
    db_connection& db_connection,
    std::string_view user_login_id
){
    const auto user_summary_exp = user_service::get_summary_by_login_id(
        db_connection,
        user_login_id
    );
    if(!user_summary_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(request, user_summary_exp.error()));
    }

    return std::move(*user_summary_exp);
}
