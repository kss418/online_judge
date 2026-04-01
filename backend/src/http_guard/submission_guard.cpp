#include "http_guard/submission_guard.hpp"

#include "common/permission_util.hpp"
#include "db_service/submission_service.hpp"

namespace{
    template <typename value_type>
    std::expected<value_type, submission_guard::response_type> require_submission_value(
        const submission_guard::request_type& request,
        std::string_view action,
        std::expected<value_type, error_code> value_exp
    ){
        if(!value_exp){
            return std::unexpected(http_response_util::create_404_or_500(
                request,
                action,
                value_exp.error()
            ));
        }

        return std::move(*value_exp);
    }
}

std::expected<submission_dto::history_list, submission_guard::response_type>
submission_guard::require_history(
    const request_type& request,
    db_connection& db_connection,
    std::int64_t submission_id
){
    return require_submission_value(
        request,
        "get submission history",
        submission_service::get_submission_history(
            db_connection,
            submission_id
        )
    );
}

std::expected<submission_dto::detail, submission_guard::response_type>
submission_guard::require_detail(
    const request_type& request,
    db_connection& db_connection,
    std::int64_t submission_id
){
    return require_submission_value(
        request,
        "get submission detail",
        submission_service::get_submission_detail(
            db_connection,
            submission_id
        )
    );
}

std::expected<submission_dto::source_detail, submission_guard::response_type>
submission_guard::require_source_detail(
    const request_type& request,
    db_connection& db_connection,
    std::int64_t submission_id
){
    return require_submission_value(
        request,
        "get submission source",
        submission_service::get_submission_source(
            db_connection,
            submission_id
        )
    );
}

std::expected<void, submission_guard::response_type> submission_guard::require_source_access(
    const request_type& request,
    const auth_dto::identity& auth_identity_value,
    const submission_dto::source_detail& source_detail_value
){
    if(
        permission_util::has_admin_access(auth_identity_value.permission_level) ||
        auth_identity_value.user_id == source_detail_value.user_id
    ){
        return {};
    }

    return std::unexpected(http_response_util::create_error(
        request,
        boost::beast::http::status::forbidden,
        "forbidden",
        "submission source access denied"
    ));
}
