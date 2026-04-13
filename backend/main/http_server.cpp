#include "http_core/http_server_bootstrap.hpp"

#include <cstdint>
#include <thread>

std::uint32_t default_http_worker_count(){
    const std::uint32_t hardware_thread_count = std::thread::hardware_concurrency();
    return hardware_thread_count == 0 ? std::uint32_t{1} : hardware_thread_count;
}

int main(){
    return http_server_bootstrap::run(default_http_worker_count());
}
