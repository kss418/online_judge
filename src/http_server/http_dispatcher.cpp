#include "http_server/http_dispatcher.hpp"
#include "http_server/http_util.hpp"

#include <string_view>
#include <utility>

std::expected<http_dispatcher, error_code> http_dispatcher::create(db_connection db_connection){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return http_dispatcher(std::move(db_connection));
}

http_dispatcher::http_dispatcher(db_connection db_connection) :
    db_connection_(std::move(db_connection)),
    auth_router_(db_connection_),
    system_router_(),
    submission_handler_(db_connection_),
    problem_handler_(db_connection_){}

http_dispatcher::http_dispatcher(http_dispatcher&& other) noexcept :
    db_connection_(std::move(other.db_connection_)),
    auth_router_(db_connection_),
    system_router_(),
    submission_handler_(db_connection_),
    problem_handler_(db_connection_){}

std::optional<std::string_view> http_dispatcher::strip_path_prefix(
    std::string_view prefix_path,
    std::string_view path
){
    if(!path.starts_with(prefix_path)){
        return std::nullopt;
    }

    path.remove_prefix(prefix_path.size());
    if(!path.empty() && path.front() != '/'){
        return std::nullopt;
    }

    return path;
}

std::optional<http_dispatcher::response_type> http_dispatcher::try_handle_route(
    const request_type& request
){
    const std::string_view path{
        request.target().data(),
        request.target().size()
    };

    const auto system_path_opt = strip_path_prefix(system_path_prefix_, path);
    if(system_path_opt){
        return system_router_.handle(request, *system_path_opt);
    }

    const auto auth_path_opt = strip_path_prefix(auth_path_prefix_, path);
    if(auth_path_opt){
        return auth_router_.handle(request, *auth_path_opt);
    }

    const auto submission_path_opt = strip_path_prefix(submission_path_prefix_, path);
    if(submission_path_opt){
        return submission_handler_.handle(request, *submission_path_opt);
    }

    const auto problem_path_opt = strip_path_prefix(problem_path_prefix_, path);
    if(problem_path_opt){
        return problem_handler_.handle(request, *problem_path_opt);
    }

    return std::nullopt;
}

http_dispatcher::response_type http_dispatcher::handle(const request_type& request){
    const auto response_opt = try_handle_route(request);
    if(response_opt.has_value()){
        return std::move(response_opt.value());
    }

    return http_util::not_found_response(request);
}
