#pragma once

#include "common/db_connection_pool.hpp"
#include "common/worker_pool.hpp"
#include "error/http_server_error.hpp"
#include "http_core/http_dispatcher.hpp"
#include "http_core/http_runtime_config.hpp"
#include "http_core/http_runtime_status_provider.hpp"
#include "http_core/request_observer.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

class http_server{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;
    using handle_callback = std::function<void(response_type)>;

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;
    http_server(http_server&&) noexcept = delete;
    http_server& operator=(http_server&&) noexcept = delete;

    static std::expected<std::shared_ptr<http_server>, http_server_error> create(
        const http_runtime_config& runtime_config
    );
    void async_handle(
        std::shared_ptr<request_type> request_ptr,
        std::string request_id,
        std::chrono::steady_clock::time_point started_at,
        handle_callback callback
    );
    response_type handle(
        const request_type& request,
        std::string request_id,
        std::chrono::steady_clock::time_point started_at
    );
    void observe_request_completion(
        const request_type& request,
        std::string_view request_id,
        const response_type& response,
        request_observer::duration_type duration
    );
    const http_runtime_config& runtime_config() const;
private:
    explicit http_server(
        http_runtime_config runtime_config,
        db_connection_pool&& db_connection_pool,
        std::unique_ptr<worker_pool> response_worker_pool
    );
    http_runtime_config runtime_config_;
    db_connection_pool db_connection_pool_;
    std::unique_ptr<worker_pool> response_worker_pool_;
    logging_request_observer request_observer_;
    http_runtime_status_provider http_runtime_status_provider_;
    http_dispatcher http_dispatcher_;
};
