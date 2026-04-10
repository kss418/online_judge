#include "http_core/http_server.hpp"

#include "http_core/http_response_util.hpp"
#include "http_core/request_context.hpp"
#include "http_core/request_id_util.hpp"

#include <chrono>
#include <utility>

std::expected<std::shared_ptr<http_server>, http_server_error> http_server::create(
    const http_runtime_config& runtime_config
){
    auto db_config_exp = db_connection::load_db_connection_config();
    if(!db_config_exp){
        return std::unexpected(http_server_error(db_config_exp.error()));
    }

    auto db_connection_pool_exp = db_connection_pool::create(
        *db_config_exp,
        runtime_config.db_pool_size
    );
    if(!db_connection_pool_exp){
        return std::unexpected(http_server_error(db_connection_pool_exp.error()));
    }

    auto response_worker_pool_exp = worker_pool::create(
        runtime_config.handler_worker_count,
        runtime_config.handler_queue_limit_opt
    );
    if(!response_worker_pool_exp){
        return std::unexpected(http_server_error(response_worker_pool_exp.error()));
    }

    return std::shared_ptr<http_server>(
        new http_server(
            runtime_config,
            std::move(*db_connection_pool_exp),
            std::move(*response_worker_pool_exp)
        )
    );
}

http_server::http_server(
    http_runtime_config runtime_config,
    db_connection_pool&& db_connection_pool,
    std::unique_ptr<worker_pool> response_worker_pool
) :
    runtime_config_(std::move(runtime_config)),
    db_connection_pool_(std::move(db_connection_pool)),
    response_worker_pool_(std::move(response_worker_pool)),
    request_observer_(),
    http_runtime_status_provider_(
        db_connection_pool_,
        *response_worker_pool_,
        runtime_config_.judge_heartbeat_stale_after
    ),
    http_dispatcher_(
        db_connection_pool_,
        runtime_config_.db_acquire_timeout_opt,
        &request_observer_,
        &http_runtime_status_provider_
    ){}

void http_server::async_handle(
    std::shared_ptr<request_type> request_ptr,
    std::string request_id,
    std::chrono::steady_clock::time_point started_at,
    handle_callback callback
){
    auto callback_ptr = std::make_shared<handle_callback>(std::move(callback));
    auto post_exp = response_worker_pool_->post(
        [this, request_ptr, request_id, started_at, callback_ptr]() mutable {
            (*callback_ptr)(handle(*request_ptr, std::move(request_id), started_at));
        }
    );
    if(post_exp){
        return;
    }

    auto response = post_exp.error().message == "worker pool queue is full"
        ? http_response_util::create_service_unavailable(
            *request_ptr,
            "handler queue is full, retry later"
        )
        : http_response_util::create_internal_server_error(
            *request_ptr,
            "dispatch_request",
            to_string(http_server_error(post_exp.error()))
        );
    request_id_util::set_response_header(response, request_id);
    observe_request_completion(
        *request_ptr,
        request_id,
        response,
        std::chrono::steady_clock::now() - started_at
    );
    (*callback_ptr)(std::move(response));
}

http_server::response_type http_server::handle(
    const request_type& request,
    std::string request_id,
    std::chrono::steady_clock::time_point started_at
){
    return http_dispatcher_.handle(request, std::move(request_id), started_at);
}

void http_server::observe_request_completion(
    const request_type& request,
    std::string_view request_id,
    const response_type& response,
    request_observer::duration_type duration
){
    request_context context(
        request,
        std::string{request_id},
        &request_observer_,
        &http_runtime_status_provider_
    );
    request_observer_.on_request_complete(context, response, duration);
}

const http_runtime_config& http_server::runtime_config() const{
    return runtime_config_;
}
