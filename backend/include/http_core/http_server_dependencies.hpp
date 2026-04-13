#pragma once

#include "common/db_connection_pool.hpp"
#include "common/worker_pool.hpp"
#include "http_core/request_observer.hpp"

#include <memory>

struct http_server_dependencies{
    db_connection_pool db_connection_pool_value;
    std::unique_ptr<worker_pool> handler_worker_pool;
    std::unique_ptr<request_observer> request_observer_ptr;
};
