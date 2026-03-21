#include "http_router/problem_router.hpp"

#include "common/string_util.hpp"
#include "http_server/http_util.hpp"

problem_router::problem_router(db_connection& db_connection) :
    db_connection_(db_connection){}

problem_router::response_type problem_router::route(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_util::not_found_response(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_problems(request);
    }

    if(path_segments.size() == 1){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_util::not_found_response(request);
        }

        return handle_get_problem(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "limits"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_util::not_found_response(request);
        }

        return handle_set_limits(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "statement"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_util::not_found_response(request);
        }

        return handle_set_statement(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "testcases"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_util::not_found_response(request);
        }

        return handle_create_testcases(request, *problem_id_opt);
    }

    return http_util::not_found_response(request);
}

problem_router::response_type problem_router::handle_problems(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return problem_handler::handle_list_problems_get(request, db_connection_);
    }

    if(request.method() == boost::beast::http::verb::post){
        return problem_handler::handle_create_problem_post(request, db_connection_);
    }

    return http_util::method_not_allowed_response(request);
}

problem_router::response_type problem_router::handle_get_problem(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::get){
        return problem_handler::handle_get_problem_get(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_util::method_not_allowed_response(request);
}

problem_router::response_type problem_router::handle_set_limits(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_handler::handle_set_limits_put(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_util::method_not_allowed_response(request);
}

problem_router::response_type problem_router::handle_set_statement(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_handler::handle_set_statement_put(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_util::method_not_allowed_response(request);
}

problem_router::response_type problem_router::handle_create_testcases(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::post){
        return problem_handler::handle_create_testcase_post(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_util::method_not_allowed_response(request);
}
