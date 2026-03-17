#include "http_handler/problem_handler.hpp"
#include "common/string_util.hpp"
#include "http_server/http_util.hpp"

#include "db/problem_core_service.hpp"

#include <boost/json.hpp>

problem_handler::problem_handler(db_connection& db_connection) :
    db_connection_(db_connection){}

bool problem_handler::is_problem_path(std::string_view path){
    return path.starts_with(path_prefix_);
}

problem_handler::response_type problem_handler::handle(
    const request_type& request,
    std::string_view path
){
    const auto path_segments_opt = http_util::parse_path("", path);
    if(!path_segments_opt){
        return http_util::not_found_response(request);
    }

    const auto& path_segments = *path_segments_opt;
    if(path_segments.empty()){
        if(request.method() == boost::beast::http::verb::post){
            return handle_create_problem_post(request);
        }

        return http_util::method_not_allowed_response(request);
    }

    if(path_segments.size() == 2 && path_segments[1] == "limits"){
        const auto problem_id_opt = string_util::parse_positive_int64(path_segments[0]);
        if(!problem_id_opt){
            return http_util::not_found_response(request);
        }

        if(request.method() == boost::beast::http::verb::put){
            return handle_set_limits_put(request, *problem_id_opt);
        }

        return http_util::method_not_allowed_response(request);
    }

    return http_util::not_found_response(request);
}

problem_handler::response_type problem_handler::handle_create_problem_post(
    const request_type& request
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto create_problem_exp = problem_core_service::create_problem(db_connection_);
    if(!create_problem_exp){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to create problem: " + to_string(create_problem_exp.error()) + "\n"
        );
    }

    boost::json::object response_object;
    response_object["problem_id"] = create_problem_exp.value();

    auto response = http_util::create_text_response(
        request,
        boost::beast::http::status::created,
        boost::json::serialize(response_object) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

problem_handler::response_type problem_handler::handle_set_limits_put(
    const request_type& request,
    std::int64_t problem_id
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto& request_object = *request_object_opt;
    const auto memory_limit_mb_opt = http_util::get_positive_int32_field(
        request_object,
        "memory_limit_mb"
    );
    const auto time_limit_ms_opt = http_util::get_positive_int32_field(
        request_object,
        "time_limit_ms"
    );
    if(!memory_limit_mb_opt || !time_limit_ms_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: memory_limit_mb, time_limit_ms\n"
        );
    }

    limits limits_value;
    limits_value.memory_limit_mb = *memory_limit_mb_opt;
    limits_value.time_limit_ms = *time_limit_ms_opt;

    const auto set_limits_exp = problem_core_service::set_limits(
        db_connection_,
        problem_id,
        limits_value
    );
    if(!set_limits_exp){
        const auto code = set_limits_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to set problem limits: " + to_string(code) + "\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "problem limits updated\n"
    );
}
