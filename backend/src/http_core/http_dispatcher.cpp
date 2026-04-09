#include "http_core/http_dispatcher.hpp"
#include "http_core/request_parser.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/request_id_util.hpp"

#include <chrono>
#include <string_view>

namespace{
    using request_clock = std::chrono::steady_clock;

    constexpr std::chrono::milliseconds DB_CONNECTION_ACQUIRE_TIMEOUT{100};

    http_dispatcher::response_type finalize_response(
        request_context& context,
        http_dispatcher::response_type response,
        request_clock::time_point started_at
    ){
        request_id_util::set_response_header(response, context.request_id);
        if(context.observer != nullptr){
            context.observer->on_request_complete(
                context,
                response,
                request_clock::now() - started_at
            );
        }

        return response;
    }
}

http_dispatcher::http_dispatcher(
    db_connection_pool& db_connection_pool,
    request_observer* request_observer
) :
    db_connection_pool_(db_connection_pool),
    request_observer_(request_observer),
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
    request_context& context,
    std::string_view path
){
    const auto system_path_opt = strip_path_prefix(system_path_prefix_, path);
    if(system_path_opt){
        return system_router_.route(context, *system_path_opt);
    }

    return std::nullopt;
}

std::optional<http_dispatcher::response_type> http_dispatcher::try_handle_route(
    request_context& context,
    std::string_view path
){
    const auto auth_path_opt = strip_path_prefix(auth_path_prefix_, path);
    if(auth_path_opt){
        auth_router auth_router_value;
        return auth_router_value.route(context, *auth_path_opt);
    }

    const auto submission_path_opt = strip_path_prefix(submission_path_prefix_, path);
    if(submission_path_opt){
        submission_router submission_router_value;
        return submission_router_value.route(context, *submission_path_opt);
    }

    const auto problem_path_opt = strip_path_prefix(problem_path_prefix_, path);
    if(problem_path_opt){
        problem_router problem_router_value;
        return problem_router_value.route(context, *problem_path_opt);
    }

    const auto user_path_opt = strip_path_prefix(user_path_prefix_, path);
    if(user_path_opt){
        user_router user_router_value;
        return user_router_value.route(context, *user_path_opt);
    }

    return std::nullopt;
}

http_dispatcher::response_type http_dispatcher::handle(const request_type& request){
    const auto started_at = request_clock::now();
    const std::string request_id = request_id_util::make_request_id();
    const std::string_view target{
        request.target().data(),
        request.target().size()
    };
    const auto path = request_parser::get_target_path(target);
    request_context context(request, request_id, request_observer_);

    const auto system_response_opt = try_handle_system_route(context, path);
    if(system_response_opt.has_value()){
        return finalize_response(
            context,
            std::move(system_response_opt.value()),
            started_at
        );
    }

    if(!has_db_route_prefix(path)){
        return finalize_response(
            context,
            http_response_util::create_not_found(request),
            started_at
        );
    }

    auto db_connection_lease_exp = db_connection_pool_.acquire_for(DB_CONNECTION_ACQUIRE_TIMEOUT);
    if(!db_connection_lease_exp){
        if(db_connection_lease_exp.error() == pool_error::timed_out){
            return finalize_response(
                context,
                http_response_util::create_service_unavailable(
                    request,
                    "db connection pool is busy, retry later"
                ),
                started_at
            );
        }

        return finalize_response(
            context,
            http_response_util::create_internal_server_error(
                request,
                "acquire_db_connection",
                to_string(db_connection_lease_exp.error())
            ),
            started_at
        );
    }

    request_context db_context(
        request,
        std::move(*db_connection_lease_exp),
        request_id,
        request_observer_
    );
    const auto response_opt = try_handle_route(db_context, path);
    if(response_opt.has_value()){
        return finalize_response(
            db_context,
            std::move(response_opt.value()),
            started_at
        );
    }

    return finalize_response(
        db_context,
        http_response_util::create_not_found(request),
        started_at
    );
}
