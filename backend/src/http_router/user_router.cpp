#include "http_router/user_router.hpp"

#include "common/string_util.hpp"
#include "http_core/http_util.hpp"

user_router::user_router(db_connection& db_connection) :
    db_connection_(db_connection){}

user_router::response_type user_router::route(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_user_list(request);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "statistics"){
        return handle_user_me_statistics(request);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "solved-problems"){
        return handle_user_me_solved_problems(request);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "wrong-problems"){
        return handle_user_me_wrong_problems(request);
    }

    if(path_segments.size() == 1 && path_segments[0] == "me"){
        return handle_user_me(request);
    }

    if(path_segments.size() == 2 && path_segments[1] == "admin"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_user_admin(request, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "user"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_user_regular(request, *user_id_opt);
    }

    return http_response_util::create_not_found(request);
}

user_router::response_type user_router::handle_user_list(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_list(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_me(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return user_handler::get_me(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_me_statistics(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_submission_statistics(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_me_solved_problems(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_solved_problems(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_me_wrong_problems(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_wrong_problems(
            request,
            db_connection_
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_admin(
    const request_type& request,
    std::int64_t user_id
){
    if(request.method() == boost::beast::http::verb::put){
        return user_handler::put_user_admin(
            request,
            db_connection_,
            user_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

user_router::response_type user_router::handle_user_regular(
    const request_type& request,
    std::int64_t user_id
){
    if(request.method() == boost::beast::http::verb::put){
        return user_handler::put_user_regular(
            request,
            db_connection_,
            user_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}
