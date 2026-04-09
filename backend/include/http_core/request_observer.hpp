#pragma once

#include "http_core/http_response_util.hpp"

#include <chrono>

struct request_context;

class request_observer{
public:
    using duration_type = std::chrono::steady_clock::duration;

    request_observer() = default;
    request_observer(const request_observer&) = delete;
    request_observer& operator=(const request_observer&) = delete;
    request_observer(request_observer&&) noexcept = delete;
    request_observer& operator=(request_observer&&) noexcept = delete;
    virtual ~request_observer() = default;

    virtual void on_request_complete(
        const request_context& context,
        const http_response_util::response_type& response,
        duration_type duration
    ) noexcept = 0;
};

class logging_request_observer final : public request_observer{
public:
    void on_request_complete(
        const request_context& context,
        const http_response_util::response_type& response,
        duration_type duration
    ) noexcept override;
};
