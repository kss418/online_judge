#include "http_core/http_dispatcher.hpp"
#include "http_core/request_parser.hpp"
#include "http_core/http_response_util.hpp"

#include <chrono>
#include <string_view>

namespace{
    constexpr std::chrono::milliseconds DB_CONNECTION_ACQUIRE_TIMEOUT{100};
}

http_dispatcher::http_dispatcher(db_connection_pool& db_connection_pool) :
    db_connection_pool_(db_connection_pool),
    system_router_(){}

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

bool http_dispatcher::has_db_route_prefix(std::string_view path){
    return
        strip_path_prefix(auth_path_prefix_, path).has_value() ||
        strip_path_prefix(submission_path_prefix_, path).has_value() ||
        strip_path_prefix(problem_path_prefix_, path).has_value() ||
        strip_path_prefix(user_path_prefix_, path).has_value();
}

std::optional<http_dispatcher::response_type> http_dispatcher::try_handle_system_route(
    const request_type& request,
    std::string_view path
){
    const auto system_path_opt = strip_path_prefix(system_path_prefix_, path);
    if(system_path_opt){
        return system_router_.route(request, *system_path_opt);
    }

    return std::nullopt;
}

std::optional<http_dispatcher::response_type> http_dispatcher::try_handle_route(
    const request_type& request,
    std::string_view path,
    db_connection& db_connection
){
    const auto auth_path_opt = strip_path_prefix(auth_path_prefix_, path);
    if(auth_path_opt){
        auth_router auth_router_value(db_connection);
        return auth_router_value.route(request, *auth_path_opt);
    }

    const auto submission_path_opt = strip_path_prefix(submission_path_prefix_, path);
    if(submission_path_opt){
        submission_router submission_router_value(db_connection);
        return submission_router_value.route(request, *submission_path_opt);
    }

    const auto problem_path_opt = strip_path_prefix(problem_path_prefix_, path);
    if(problem_path_opt){
        problem_router problem_router_value(db_connection);
        return problem_router_value.route(request, *problem_path_opt);
    }

    const auto user_path_opt = strip_path_prefix(user_path_prefix_, path);
    if(user_path_opt){
        user_router user_router_value(db_connection);
        return user_router_value.route(request, *user_path_opt);
    }

    return std::nullopt;
}

http_dispatcher::response_type http_dispatcher::handle(const request_type& request){
    const std::string_view target{
        request.target().data(),
        request.target().size()
    };
    const auto path = request_parser::get_target_path(target);

    const auto system_response_opt = try_handle_system_route(request, path);
    if(system_response_opt.has_value()){
        return std::move(system_response_opt.value());
    }

    if(!has_db_route_prefix(path)){
        return http_response_util::create_not_found(request);
    }

    auto db_connection_lease_exp = db_connection_pool_.acquire_for(DB_CONNECTION_ACQUIRE_TIMEOUT);
    if(!db_connection_lease_exp){
        if(db_connection_lease_exp.error() == pool_error::timed_out){
            return http_response_util::create_error(
                request,
                http_error{
                    http_error_code::service_unavailable,
                    "db connection pool is busy, retry later"
                }
            );
        }

        return http_response_util::create_error(
            request,
            http_error{
                http_error_code::internal_server_error,
                "failed to acquire db connection: " + to_string(db_connection_lease_exp.error())
            }
        );
    }

    const auto response_opt = try_handle_route(
        request,
        path,
        db_connection_lease_exp->connection()
    );
    if(response_opt.has_value()){
        return std::move(response_opt.value());
    }

    return http_response_util::create_not_found(request);
}
