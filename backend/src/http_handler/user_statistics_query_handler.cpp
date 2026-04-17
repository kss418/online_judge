#include "http_handler/user_statistics_query_handler.hpp"

#include "db_service/user_statistics_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_handler/user_path_spec_helper.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    auto make_get_me_submission_statistics_spec(){
        return http_handler_spec::make_auth_user_id_json_spec(
            user_statistics_service::get_submission_statistics,
            user_json_serializer::make_submission_statistics_object
        );
    }

    auto make_get_user_submission_statistics_spec(std::int64_t user_id){
        return http_handler_spec::make_existing_user_json_spec(
            user_id,
            user_statistics_service::get_submission_statistics,
            user_json_serializer::make_submission_statistics_object
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
