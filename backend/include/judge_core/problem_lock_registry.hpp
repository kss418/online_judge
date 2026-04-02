#pragma once

#include "error/error_code.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <unordered_map>

class problem_lock_registry{
    struct entry;

public:
    class scoped_lock{
    public:
        scoped_lock(const scoped_lock&) = delete;
        scoped_lock& operator=(const scoped_lock&) = delete;
        scoped_lock(scoped_lock&&) noexcept = default;
        scoped_lock& operator=(scoped_lock&&) noexcept = default;
        ~scoped_lock() = default;

    private:
        friend class problem_lock_registry;

        scoped_lock(std::shared_ptr<entry> entry_value, std::unique_lock<std::mutex> lock_value);

        std::shared_ptr<entry> entry_;
        std::unique_lock<std::mutex> lock_;
    };

    std::expected<scoped_lock, error_code> lock(std::int64_t problem_id);

private:
    struct entry{
        std::mutex mutex_;
    };

    std::mutex registry_mutex_;
    std::unordered_map<std::int64_t, std::weak_ptr<entry>> entries_;
};
