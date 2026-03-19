#include "http_router/submission_router.hpp"

#include "common/string_util.hpp"
#include "http_server/http_util.hpp"

submission_router::submission_router(db_connection& db_connection) :
    db_connection_(db_connection){}

submission_router::response_type submission_router::route(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_util::not_found_response(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_submissions(request);
    }

    if(path_segments.size() == 1){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_util::not_found_response(request);
        }

        return handle_submission(request, *resource_id_opt);
    }

    return http_util::not_found_response(request);
}

submission_router::response_type submission_router::handle_submissions(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::get){
        return submission_handler::handle_list_submissions_get(
            request,
            db_connection_
        );
    }

    return http_util::method_not_allowed_response(request);
}

submission_router::response_type submission_router::handle_submission(
    const request_type& request,
    std::int64_t resource_id
){
    if(request.method() == boost::beast::http::verb::post){
        return submission_handler::handle_create_submission_post(
            request,
            db_connection_,
            resource_id
        );
    }

    return http_util::method_not_allowed_response(request);
}
