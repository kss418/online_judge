#pragma once

#include "error/pool_error.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <expected>
#include <memory>
#include <utility>

class worker_pool{
public:
    worker_pool(const worker_pool&) = delete;
    worker_pool& operator=(const worker_pool&) = delete;
    worker_pool(worker_pool&&) noexcept = delete;
    worker_pool& operator=(worker_pool&&) noexcept = delete;

    static std::expected<std::unique_ptr<worker_pool>, pool_error> create(std::size_t worker_count);

    template <typename task_type>
    std::expected<void, pool_error> post(task_type&& task){
        try{
            boost::asio::post(thread_pool_, std::forward<task_type>(task));
            return {};
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
private:
    explicit worker_pool(std::size_t worker_count);

    boost::asio::thread_pool thread_pool_;
};
