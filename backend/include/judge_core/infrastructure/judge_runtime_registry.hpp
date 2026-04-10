#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

class judge_runtime_registry{
public:
    class busy_worker_guard{
    public:
        busy_worker_guard() = default;
        busy_worker_guard(const busy_worker_guard&) = delete;
        busy_worker_guard& operator=(const busy_worker_guard&) = delete;

        busy_worker_guard(busy_worker_guard&& other) noexcept :
            registry_(other.registry_){
            other.registry_ = nullptr;
        }

        busy_worker_guard& operator=(busy_worker_guard&& other) noexcept{
            if(this == &other){
                return *this;
            }

            release();
            registry_ = other.registry_;
            other.registry_ = nullptr;
            return *this;
        }

        ~busy_worker_guard(){
            release();
        }

    private:
        friend class judge_runtime_registry;

        explicit busy_worker_guard(judge_runtime_registry* registry) :
            registry_(registry){}

        void release(){
            if(registry_ == nullptr){
                return;
            }

            registry_->busy_worker_count_.fetch_sub(1, std::memory_order_acq_rel);
            registry_ = nullptr;
        }

        judge_runtime_registry* registry_ = nullptr;
    };

    explicit judge_runtime_registry(std::int64_t configured_worker_count) :
        configured_worker_count_(configured_worker_count){}

    std::int64_t configured_worker_count() const noexcept{
        return configured_worker_count_;
    }

    std::int64_t busy_worker_count() const noexcept{
        return busy_worker_count_.load(std::memory_order_relaxed);
    }

    std::int64_t snapshot_cache_hit_count() const noexcept{
        return snapshot_cache_hit_count_.load(std::memory_order_relaxed);
    }

    std::int64_t snapshot_cache_miss_count() const noexcept{
        return snapshot_cache_miss_count_.load(std::memory_order_relaxed);
    }

    busy_worker_guard mark_worker_busy(){
        busy_worker_count_.fetch_add(1, std::memory_order_acq_rel);
        return busy_worker_guard(this);
    }

    void record_snapshot_cache_hit() noexcept{
        snapshot_cache_hit_count_.fetch_add(1, std::memory_order_acq_rel);
    }

    void record_snapshot_cache_miss() noexcept{
        snapshot_cache_miss_count_.fetch_add(1, std::memory_order_acq_rel);
    }

private:
    std::int64_t configured_worker_count_ = 0;
    std::atomic<std::int64_t> busy_worker_count_{0};
    std::atomic<std::int64_t> snapshot_cache_hit_count_{0};
    std::atomic<std::int64_t> snapshot_cache_miss_count_{0};
};
