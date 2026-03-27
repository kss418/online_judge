#include "http_router/submission_router.hpp"

#include "common/string_util.hpp"
#include "http_core/http_util.hpp"

submission_router::submission_router(db_connection& db_connection) :
    db_connection_(db_connection){}

submission_router::response_type submission_router::route(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_submissions(request);
    }

    if(path_segments.size() == 1){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_submission(request, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "history"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_submission_history(request, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "source"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_submission_source(request, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "rejudge"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_submission_rejudge(request, *resource_id_opt);
    }

    return http_response_util::create_not_found(request);
}

submission_router::response_type submission_router::handle_submissions(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submissions(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

submission_router::response_type submission_router::handle_submission(
    const request_type& request,
    std::int64_t resource_id
){
    if(request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission(
            request,
            db_connection_,
            resource_id
        );
    }

    if(request.method() == boost::beast::http::verb::post){
        return submission_handler::post_submission(
            request,
            db_connection_,
            resource_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

submission_router::response_type submission_router::handle_submission_history(
    const request_type& request,
    std::int64_t resource_id
){
    if(request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission_history(
            request,
            db_connection_,
            resource_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

submission_router::response_type submission_router::handle_submission_source(
    const request_type& request,
    std::int64_t resource_id
){
    if(request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission_source(
            request,
            db_connection_,
            resource_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

submission_router::response_type submission_router::handle_submission_rejudge(
    const request_type& request,
    std::int64_t resource_id
){
    if(request.method() == boost::beast::http::verb::post){
        return submission_handler::post_submission_rejudge(
            request,
            db_connection_,
            resource_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}
