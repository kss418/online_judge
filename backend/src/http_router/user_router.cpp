#include "http_router/user_router.hpp"

#include "common/string_util.hpp"
#include "http_core/request_parser.hpp"
#include "http_core/http_response_util.hpp"

user_router::response_type user_router::route(
    context_type& context,
    std::string_view path
){
    const auto path_segments_opt = request_parser::parse_path("", path);
    if(!path_segments_opt){
        return http_response_util::create_not_found(context.request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        return handle_user_list(context);
    }

    if(path_segments.size() == 1 && path_segments[0] == "list"){
        return handle_public_user_list(context);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "statistics"){
        return handle_user_me_statistics(context);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "submission-ban"){
        return handle_user_me_submission_ban(context);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "solved-problems"){
        return handle_user_me_solved_problems(context);
    }

    if(path_segments.size() == 2
        && path_segments[0] == "me"
        && path_segments[1] == "wrong-problems"){
        return handle_user_me_wrong_problems(context);
    }

    if(path_segments.size() == 1 && path_segments[0] == "me"){
        return handle_user_me(context);
    }

    if(path_segments.size() == 2 && path_segments[0] == "id"){
        const auto decoded_user_login_id_opt = string_util::decode_percent_encoded(
            path_segments[1]
        );
        if(
            !decoded_user_login_id_opt ||
            decoded_user_login_id_opt->empty()
        ){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_summary_by_login_id(
            context,
            *decoded_user_login_id_opt
        );
    }

    if(path_segments.size() == 2 && path_segments[1] == "statistics"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_statistics(context, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "solved-problems"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_solved_problems(context, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "wrong-problems"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_wrong_problems(context, *user_id_opt);
    }

    if(path_segments.size() == 1){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_summary(context, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "admin"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_admin(context, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "user"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_regular(context, *user_id_opt);
    }

    if(path_segments.size() == 2 && path_segments[1] == "submission-ban"){
        const auto user_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!user_id_opt){
            return http_response_util::create_not_found(context.request);
        }

        return handle_user_submission_ban(context, *user_id_opt);
    }

    return http_response_util::create_not_found(context.request);
}

user_router::response_type user_router::handle_user_list(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_list(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_public_user_list(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_public_user_list(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_me(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_me(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_me_statistics(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_submission_statistics(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_me_submission_ban(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_submission_ban(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_me_solved_problems(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_solved_problems(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_me_wrong_problems(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_me_wrong_problems(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_summary_by_login_id(
    context_type& context,
    const std::string& user_login_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_summary_by_login_id(context, user_login_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_summary(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_summary(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_statistics(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_submission_statistics(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_solved_problems(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_solved_problems(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_wrong_problems(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_wrong_problems(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_admin(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::put){
        return user_handler::put_user_admin(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_regular(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::put){
        return user_handler::put_user_regular(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

user_router::response_type user_router::handle_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    if(context.request.method() == boost::beast::http::verb::get){
        return user_handler::get_user_submission_ban(context, user_id);
    }

    if(context.request.method() == boost::beast::http::verb::post){
        return user_handler::post_user_submission_ban(context, user_id);
    }

    if(context.request.method() == boost::beast::http::verb::delete_){
        return user_handler::delete_user_submission_ban(context, user_id);
    }

    return http_response_util::create_method_not_allowed(context.request);
}
