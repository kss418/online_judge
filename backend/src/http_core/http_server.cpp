#include "http_core/http_server.hpp"

#include "common/string_util.hpp"

#include <cstdlib>
#include <limits>
#include <utility>

namespace{
    std::expected<std::size_t, error_code> resolve_http_db_pool_size(
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
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return static_cast<std::size_t>(*pool_size_opt);
    }
}

std::expected<std::shared_ptr<http_server>, error_code> http_server::create(
    std::size_t default_db_connection_pool_size
){
    auto pool_size_exp = resolve_http_db_pool_size(default_db_connection_pool_size);
    if(!pool_size_exp){
        return std::unexpected(pool_size_exp.error());
    }

    auto db_connection_pool_exp = db_connection_pool::create(*pool_size_exp);
    if(!db_connection_pool_exp){
        return std::unexpected(db_connection_pool_exp.error());
    }

    return std::shared_ptr<http_server>(new http_server(std::move(*db_connection_pool_exp)));
}

http_server::http_server(db_connection_pool&& db_connection_pool) :
    db_connection_pool_(std::move(db_connection_pool)),
    http_dispatcher_(db_connection_pool_){}

http_server::response_type http_server::handle(const request_type& request){
    return http_dispatcher_.handle(request);
}
