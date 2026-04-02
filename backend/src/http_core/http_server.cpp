#include "http_core/http_server.hpp"

#include "common/string_util.hpp"
#include "error/infra_error.hpp"
#include "http_core/http_response_util.hpp"

#include <cstdlib>
#include <limits>
#include <utility>

namespace{
    std::expected<std::size_t, infra_error> resolve_http_db_pool_size(
        std::size_t default_http_db_pool_size
    ){
        const char* pool_size_text = std::getenv("HTTP_DB_POOL_SIZE");
        if(pool_size_text == nullptr || *pool_size_text == '\0'){
            return default_http_db_pool_size;
        }

        const auto pool_size_opt = string_util::parse_positive_int64(pool_size_text);
        if(
            !pool_size_opt ||
            *pool_size_opt > static_cast<std::int64_t>(std::numeric_limits<std::size_t>::max())
        ){
            return std::unexpected(infra_error::invalid_argument);
        }

        return static_cast<std::size_t>(*pool_size_opt);
    }

    std::expected<std::size_t, infra_error> resolve_http_handler_worker_count(
        std::size_t default_http_worker_count
    ){
        const char* worker_count_text = std::getenv("HTTP_HANDLER_WORKER_COUNT");
        if(worker_count_text == nullptr || *worker_count_text == '\0'){
            return default_http_worker_count;
        }

        const auto worker_count_opt = string_util::parse_positive_int64(worker_count_text);
        if(
            !worker_count_opt ||
            *worker_count_opt > static_cast<std::int64_t>(std::numeric_limits<std::size_t>::max())
        ){
            return std::unexpected(infra_error::invalid_argument);
        }

        return static_cast<std::size_t>(*worker_count_opt);
    }
}

std::expected<std::shared_ptr<http_server>, http_server_error> http_server::create(
    std::size_t default_http_worker_count
){
    auto pool_size_exp = resolve_http_db_pool_size(default_http_worker_count);
    if(!pool_size_exp){
        return std::unexpected(http_server_error(pool_size_exp.error()));
    }

    auto handler_worker_count_exp = resolve_http_handler_worker_count(default_http_worker_count);
    if(!handler_worker_count_exp){
        return std::unexpected(http_server_error(handler_worker_count_exp.error()));
    }

    auto db_config_exp = db_connection::load_db_connection_config();
    if(!db_config_exp){
        return std::unexpected(http_server_error(db_config_exp.error()));
    }

    auto db_connection_pool_exp = db_connection_pool::create(*db_config_exp, *pool_size_exp);
    if(!db_connection_pool_exp){
        return std::unexpected(http_server_error(db_connection_pool_exp.error()));
    }

    auto response_worker_pool_exp = worker_pool::create(*handler_worker_count_exp);
    if(!response_worker_pool_exp){
        return std::unexpected(http_server_error(response_worker_pool_exp.error()));
    }

    return std::shared_ptr<http_server>(
        new http_server(
            std::move(*db_connection_pool_exp),
            std::move(*response_worker_pool_exp)
        )
    );
}

http_server::http_server(
    db_connection_pool&& db_connection_pool,
    std::unique_ptr<worker_pool> response_worker_pool
) :
    db_connection_pool_(std::move(db_connection_pool)),
    response_worker_pool_(std::move(response_worker_pool)),
    http_dispatcher_(db_connection_pool_){}

void http_server::async_handle(request_type request, handle_callback callback){
    auto request_ptr = std::make_shared<request_type>(std::move(request));
    auto post_exp = response_worker_pool_->post(
        [this, request_ptr, callback = std::move(callback)]() mutable {
            callback(handle(*request_ptr));
        }
    );
    if(post_exp){
        return;
    }

    const http_server_error dispatch_error(post_exp.error());
    callback(
        http_response_util::create_error(
            *request_ptr,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to dispatch request: " + to_string(dispatch_error)
        )
    );
}

http_server::response_type http_server::handle(const request_type& request){
    return http_dispatcher_.handle(request);
}
