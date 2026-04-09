#include "http_router/submission_router.hpp"

#include "common/string_util.hpp"
#include "http_core/request_parser.hpp"
#include "http_core/http_response_util.hpp"

submission_router::response_type submission_router::route(
    context_type& context,
    std::string_view path
){
    const auto path_segments_opt = request_parser::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(context.request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_submissions(context);
    }

    if(path_segments.size() == 2 && path_segments[0] == "status" && path_segments[1] == "batch"){
        return handle_submission_status_batch(context);
    }

    if(path_segments.size() == 1){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_submission(context, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "history"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_submission_history(context, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "source"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_submission_source(context, *resource_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "rejudge"){
        const auto resource_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!resource_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_submission_rejudge(context, *resource_id_opt);
    }

    return http_response_util::create_not_found(context.request);
}

submission_router::response_type submission_router::handle_submissions(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submissions(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

submission_router::response_type submission_router::handle_submission_status_batch(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::post){
        return submission_handler::post_submission_status_batch(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

submission_router::response_type submission_router::handle_submission(
    context_type& context,
    std::int64_t resource_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission(context, resource_id);
    }

    if(context.request.method() == boost::beast::http::verb::post){
        return submission_handler::post_submission(context, resource_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

submission_router::response_type submission_router::handle_submission_history(
    context_type& context,
    std::int64_t resource_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission_history(context, resource_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

submission_router::response_type submission_router::handle_submission_source(
    context_type& context,
    std::int64_t resource_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return submission_handler::get_submission_source(context, resource_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

submission_router::response_type submission_router::handle_submission_rejudge(
    context_type& context,
    std::int64_t resource_id
){
    if(context.request.method() == boost::beast::http::verb::post){
        return submission_handler::post_submission_rejudge(context, resource_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}
