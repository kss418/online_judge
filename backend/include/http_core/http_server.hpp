#pragma once

#include "common/db_connection_pool.hpp"
#include "common/error_code.hpp"
#include "common/worker_pool.hpp"
#include "http_core/http_dispatcher.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <functional>
#include <memory>

class http_server{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;
    using handle_callback = std::function<void(response_type)>;

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;
    http_server(http_server&&) noexcept = delete;
    http_server& operator=(http_server&&) noexcept = delete;

    static std::expected<std::shared_ptr<http_server>, error_code> create(
        std::size_t default_http_worker_count
    );
    void async_handle(request_type request, handle_callback callback);
    response_type handle(const request_type& request);
private:
    explicit http_server(
        db_connection_pool&& db_connection_pool,
        std::unique_ptr<worker_pool> response_worker_pool
    );
    db_connection_pool db_connection_pool_;
    std::unique_ptr<worker_pool> response_worker_pool_;
    http_dispatcher http_dispatcher_;
};
