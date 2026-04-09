#include "http_guard/user_guard.hpp"

#include "db_service/user_service.hpp"

std::expected<user_dto::summary, user_guard::response_type> user_guard::require_summary(
    context_type& context,
    std::int64_t user_id
){
    const auto user_summary_exp = user_service::get_summary(
        context.db_connection_ref(),
        user_id
    );
    if(!user_summary_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(
            context.request,
            user_summary_exp.error()
        ));
    }

    return std::move(*user_summary_exp);
}

std::expected<user_dto::summary, user_guard::response_type> user_guard::require_summary_by_login_id(
    context_type& context,
    std::string_view user_login_id
){
    const auto user_summary_exp = user_service::get_summary_by_login_id(
        context.db_connection_ref(),
        user_login_id
    );
    if(!user_summary_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(
            context.request,
            user_summary_exp.error()
        ));
    }

    return std::move(*user_summary_exp);
}
