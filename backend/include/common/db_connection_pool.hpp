#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"

#include <chrono>
#include <cstddef>
#include <expected>
#include <memory>

class db_connection_pool{
    struct state;

public:
    class lease{
    public:
        lease() = default;
        lease(const lease&) = delete;
        lease& operator=(const lease&) = delete;
        lease(lease&& other) noexcept;
        lease& operator=(lease&& other) noexcept;
        ~lease();

        explicit operator bool() const;
        db_connection& connection();
        const db_connection& connection() const;
        db_connection& operator*();
        const db_connection& operator*() const;
        db_connection* operator->();
        const db_connection* operator->() const;
        void release();

    private:
        friend class db_connection_pool;

        lease(std::shared_ptr<state> state_value, std::size_t slot_index);

        std::shared_ptr<state> state_;
        std::size_t slot_index_ = 0;
    };

    db_connection_pool() = default;
    db_connection_pool(const db_connection_pool&) = delete;
    db_connection_pool& operator=(const db_connection_pool&) = delete;
    db_connection_pool(db_connection_pool&&) noexcept = default;
    db_connection_pool& operator=(db_connection_pool&&) noexcept = default;

    static std::expected<db_connection_pool, error_code> create(std::size_t pool_size);

    std::expected<lease, error_code> acquire();
    std::expected<lease, error_code> acquire_for(std::chrono::milliseconds timeout);

    std::size_t size() const;
    std::size_t available_count() const;

private:
    explicit db_connection_pool(std::shared_ptr<state> state_value);

    std::expected<lease, error_code> acquire_impl(
        std::chrono::milliseconds timeout,
        bool use_timeout
    );

    std::shared_ptr<state> state_;
};
