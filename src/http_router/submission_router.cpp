#include "http_router/submission_router.hpp"

#include "http_server/http_util.hpp"

submission_router::submission_router(db_connection& db_connection) :
    db_connection_(db_connection){}

submission_router::response_type submission_router::route(
    const request_type& request,
    std::string_view path
){
    if(path.empty()){
        return handle_create_submission(request);
    }

    return http_util::not_found_response(request);
}

submission_router::response_type submission_router::handle_create_submission(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::post){
        return submission_handler::handle_create_submission_post(
            request,
            db_connection_
        );
    }

    return http_util::method_not_allowed_response(request);
}
