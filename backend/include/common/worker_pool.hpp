#pragma once

#include "error/pool_error.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <atomic>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

class worker_pool{
public:
    worker_pool(const worker_pool&) = delete;
    worker_pool& operator=(const worker_pool&) = delete;
    worker_pool(worker_pool&&) noexcept = delete;
    worker_pool& operator=(worker_pool&&) noexcept = delete;

    static std::expected<std::unique_ptr<worker_pool>, pool_error> create(
        std::size_t worker_count,
        std::optional<std::size_t> queue_limit_opt = std::nullopt
    );

    template <typename task_type>
    std::expected<void, pool_error> post(task_type&& task){
        bool queue_reserved = false;
        if(queue_limit_opt_.has_value()){
            std::size_t queued_count = queued_task_count_.load(std::memory_order_relaxed);
            while(true){
                if(queued_count >= *queue_limit_opt_){
                    return std::unexpected(
                        pool_error{
                            pool_error_code::unavailable,
                            "worker pool queue is full"
                        }
                    );
                }
                if(
                    queued_task_count_.compare_exchange_weak(
                        queued_count,
                        queued_count + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed
                    )
                ){
                    queue_reserved = true;
                    break;
                }
            }
        }

        auto wrapped_task = [this, task = std::forward<task_type>(task)]() mutable {
            if(queue_limit_opt_.has_value()){
                queued_task_count_.fetch_sub(1, std::memory_order_acq_rel);
            }
            std::invoke(std::move(task));
        };
        try{
            boost::asio::post(thread_pool_, std::move(wrapped_task));
            return {};
        }
        catch(const std::bad_alloc&){
            if(queue_reserved){
                queued_task_count_.fetch_sub(1, std::memory_order_acq_rel);
            }
            return std::unexpected(
                pool_error{pool_error_code::unavailable, "worker pool out of memory"}
            );
        }
        catch(...){
            if(queue_reserved){
                queued_task_count_.fetch_sub(1, std::memory_order_acq_rel);
            }
            return std::unexpected(pool_error::internal);
        }
    }

    std::size_t queued_task_count() const noexcept{
        return queued_task_count_.load(std::memory_order_relaxed);
    }

    std::optional<std::size_t> queue_limit_opt() const noexcept{
        return queue_limit_opt_;
    }
private:
    explicit worker_pool(
        std::size_t worker_count,
        std::optional<std::size_t> queue_limit_opt
    );

    boost::asio::thread_pool thread_pool_;
    std::optional<std::size_t> queue_limit_opt_;
    std::atomic<std::size_t> queued_task_count_{0};
};
