#include "http_router/problem_router.hpp"

#include "common/string_util.hpp"
#include "http_handler/problem_content_handler.hpp"
#include "http_handler/testcase_handler.hpp"
#include "http_core/request_parser.hpp"
#include "http_core/http_response_util.hpp"

problem_router::response_type problem_router::route(
    context_type& context,
    std::string_view path
){
    const auto path_segments_opt = request_parser::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(context.request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_problems(context);
    }

    if(path_segments.size() == 1){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_problem(context, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "limits"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_set_limits(context, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "title"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_title(context, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "statement"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_statement(context, *problem_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "sample"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_samples(context, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "sample"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        const auto sample_order_opt = string_util::parse_positive_int32(path_segments[2]);
        if(!problem_id_opt || !sample_order_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_sample(context, *problem_id_opt, *sample_order_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "testcase"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_testcases(context, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "testcase" && path_segments[2] == "all"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_testcase_all(context, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "testcase" && path_segments[2] == "zip"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_testcase_zip(context, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "testcase" && path_segments[2] == "move"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_testcase_move(context, *problem_id_opt);
    }

    if(path_segments.size() == 3 && path_segments[1] == "testcase"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        const auto testcase_order_opt = string_util::parse_positive_int32(path_segments[2]);
        if(!problem_id_opt || !testcase_order_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_testcase(context, *problem_id_opt, *testcase_order_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "rejudge"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_problem_rejudge(context, *problem_id_opt);
    }

    return http_response_util::create_not_found(context.request);
}

problem_router::response_type problem_router::handle_problems(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return problem_handler::get_problems(context);
    }

    if(context.request.method() == boost::beast::http::verb::post){
        return problem_handler::post_problem(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_problem(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return problem_handler::get_problem(context, problem_id);
    }

    if(context.request.method() == boost::beast::http::verb::delete_){
        return problem_handler::delete_problem(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_title(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::put){
        return problem_handler::put_problem(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_set_limits(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return problem_content_handler::get_limits(context, problem_id);
    }

    if(context.request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_limits(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_statement(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_statement(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_samples(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return problem_content_handler::get_samples(context, problem_id);
    }

    if(context.request.method() == boost::beast::http::verb::post){
        return problem_content_handler::post_sample(context, problem_id);
    }

    if(context.request.method() == boost::beast::http::verb::delete_){
        return problem_content_handler::delete_sample(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_sample(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t sample_order
){
    if(context.request.method() == boost::beast::http::verb::put){
        return problem_content_handler::put_sample(context, problem_id, sample_order);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_testcases(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return testcase_handler::get_testcases(context, problem_id);
    }

    if(context.request.method() == boost::beast::http::verb::post){
        return testcase_handler::post_testcase(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_testcase(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t testcase_order
){
    if(context.request.method() == boost::beast::http::verb::get){
        return testcase_handler::get_testcase(context, problem_id, testcase_order);
    }

    if(context.request.method() == boost::beast::http::verb::put){
        return testcase_handler::put_testcase(context, problem_id, testcase_order);
    }

    if(context.request.method() == boost::beast::http::verb::delete_){
        return testcase_handler::delete_testcase(context, problem_id, testcase_order);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_testcase_zip(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::post){
        return testcase_handler::post_testcase_zip(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_testcase_move(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::post){
        return testcase_handler::move_testcase(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_testcase_all(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::delete_){
        return testcase_handler::delete_all_testcases(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

problem_router::response_type problem_router::handle_problem_rejudge(
    context_type& context,
    std::int64_t problem_id
){
    if(context.request.method() == boost::beast::http::verb::post){
        return problem_handler::post_problem_rejudge(context, problem_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}
