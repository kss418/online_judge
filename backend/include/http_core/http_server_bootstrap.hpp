#pragma once

#include <cstddef>

namespace http_server_bootstrap{
    int run(std::size_t default_http_worker_count);
}
