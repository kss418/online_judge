#pragma once

#include "error/error_code.hpp"

#include <expected>

struct unique_fd{
private:
    int fd_ = -1;
public:
    unique_fd() noexcept = default;
    explicit unique_fd(int fd) noexcept;
    
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;

    unique_fd(unique_fd&& other) noexcept;
    unique_fd& operator=(unique_fd&& other) noexcept;

    ~unique_fd() noexcept;
    void close(int new_fd = -1) noexcept;
    std::expected<void, error_code> close_checked() noexcept;
    int get() const noexcept;
    explicit operator bool() const noexcept;
};
