#include "common/worker_pool.hpp"

std::expected<std::unique_ptr<worker_pool>, error_code> worker_pool::create(std::size_t worker_count){
    if(worker_count == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        return std::unique_ptr<worker_pool>(new worker_pool(worker_count));
    }
    catch(const std::bad_alloc&){
        return std::unexpected(error_code::create(errno_error::out_of_memory));
    }
    catch(...){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }
}

worker_pool::worker_pool(std::size_t worker_count) :
    thread_pool_(worker_count){}
