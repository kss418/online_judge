#include "common/worker_pool.hpp"

std::expected<std::unique_ptr<worker_pool>, pool_error> worker_pool::create(
    std::size_t worker_count,
    std::optional<std::size_t> queue_limit_opt
){
    if(worker_count == 0){
        return std::unexpected(pool_error::invalid_argument);
    }

    try{
        return std::unique_ptr<worker_pool>(
            new worker_pool(worker_count, queue_limit_opt)
        );
    }
    catch(const std::bad_alloc&){
        return std::unexpected(
            pool_error{pool_error_code::unavailable, "worker pool out of memory"}
        );
    }
    catch(...){
        return std::unexpected(pool_error::internal);
    }
}

worker_pool::worker_pool(
    std::size_t worker_count,
    std::optional<std::size_t> queue_limit_opt
) :
    thread_pool_(worker_count),
    queue_limit_opt_(queue_limit_opt){}
