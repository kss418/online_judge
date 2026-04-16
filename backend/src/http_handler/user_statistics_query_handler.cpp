#include "http_handler/user_statistics_query_handler.hpp"

#include "db_service/user_statistics_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/handler_spec_helper.hpp"
#include "http_guard/user_guard.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_me_submission_statistics_spec(){
        return http_handler_spec::make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<std::int64_t> {
                return auth_identity_value.user_id;
            },
            user_statistics_service::get_submission_statistics,
            user_json_serializer::make_submission_statistics_object
        );
    }

    auto make_get_user_submission_statistics_spec(std::int64_t user_id){
        return http_handler_spec::make_single_path_value_json_spec(
            user_id,
            [](const http_guard::guard_context&, std::int64_t user_id)
                -> command_expected<std::int64_t> {
                return user_id;
            },
            user_statistics_service::get_submission_statistics,
            user_json_serializer::make_submission_statistics_object,
            user_guard::make_exists_guard(user_id)
        );
    }
}

response_type user_query_handler::get_me_submission_statistics(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_submission_statistics_spec());
}

response_type user_query_handler::get_user_submission_statistics(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_submission_statistics_spec(user_id)
    );
}
