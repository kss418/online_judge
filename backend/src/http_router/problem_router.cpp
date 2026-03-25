#include "http_router/problem_router.hpp"

#include "common/string_util.hpp"
#include "http_handler/problem_content_handler.hpp"
#include "http_handler/testcase_handler.hpp"
#include "http_server/http_util.hpp"

problem_router::problem_router(db_connection& db_connection) :
    db_connection_(db_connection){}

problem_router::response_type problem_router::route(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_problems(request);
    }

    if(path_segments.size() == 1){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_problem(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "limits"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_set_limits(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "title"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_title(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "statement"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_statement(request, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "sample"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_samples(request, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "sample"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        const auto sample_order_opt = string_util::parse_positive_int32(path_segments[2]);
        if(!problem_id_opt || !sample_order_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_sample(request, *problem_id_opt, *sample_order_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "testcase"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_testcases(request, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "testcase"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        const auto testcase_order_opt = string_util::parse_positive_int32(path_segments[2]);
        if(!problem_id_opt || !testcase_order_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_testcase(request, *problem_id_opt, *testcase_order_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "rejudge"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(request);
        }

        return handle_problem_rejudge(request, *problem_id_opt);
    }

    return http_response_util::create_not_found(request);
}

problem_router::response_type problem_router::handle_problems(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return problem_handler::get_problems(request, db_connection_);
    }

    if(request.method() == boost::beast::http::verb::post){
        return problem_handler::post_problem(request, db_connection_);
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_problem(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::get){
        return problem_handler::get_problem(
            request,
            db_connection_,
            problem_id
        );
    }

    if(request.method() == boost::beast::http::verb::delete_){
        return problem_handler::delete_problem(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_title(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_handler::put_problem(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_set_limits(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_limits(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_statement(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_statement(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_samples(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::get){
        return problem_content_handler::get_samples(
            request,
            db_connection_,
            problem_id
        );
    }

    if(request.method() == boost::beast::http::verb::post){
        return problem_content_handler::post_sample(
            request,
            db_connection_,
            problem_id
        );
    }

    if(request.method() == boost::beast::http::verb::delete_){
        return problem_content_handler::delete_sample(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_sample(
    const request_type& request,
    std::int64_t problem_id,
    std::int32_t sample_order
){
    if(request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_sample(
            request,
            db_connection_,
            problem_id,
            sample_order
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_testcases(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::get){
        return testcase_handler::get_testcases(
            request,
            db_connection_,
            problem_id
        );
    }

    if(request.method() == boost::beast::http::verb::post){
        return testcase_handler::post_testcase(
            request,
            db_connection_,
            problem_id
        );
    }

    if(request.method() == boost::beast::http::verb::delete_){
        return testcase_handler::delete_testcase(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_testcase(
    const request_type& request,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    if(request.method() == boost::beast::http::verb::put){
        return testcase_handler::put_testcase(
            request,
            db_connection_,
            problem_id,
            testcase_order
        );
    }

    return http_response_util::create_method_not_allowed(request);
}

problem_router::response_type problem_router::handle_problem_rejudge(
    const request_type& request,
    std::int64_t problem_id
){
    if(request.method() == boost::beast::http::verb::post){
        return problem_handler::post_problem_rejudge(
            request,
            db_connection_,
            problem_id
        );
    }

    return http_response_util::create_method_not_allowed(request);
}
