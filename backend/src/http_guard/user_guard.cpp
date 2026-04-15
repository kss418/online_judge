#include "http_guard/user_guard.hpp"

#include "db_service/user_service.hpp"

std::expected<void, user_guard::response_type> user_guard::require_exists(
    context_type& context,
    std::int64_t user_id
){
    const auto ensure_user_exists_exp = user_service::ensure_user_exists(
        context.db_connection_ref(),
        user_id
    );
    if(!ensure_user_exists_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(
            context.request,
            ensure_user_exists_exp.error()
        ));
    }

    return {};
}
