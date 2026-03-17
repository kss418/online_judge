#include "http_handler/problem_handler.hpp"
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
    if(path.empty()){
        if(request.method() == boost::beast::http::verb::post){
            return handle_create_problem_post(request);
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::method_not_allowed,
            "method not allowed\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::not_found,
        "not found\n"
    );
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
