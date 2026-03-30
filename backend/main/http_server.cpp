#include "http_core/acceptor.hpp"
#include "http_core/http_server.hpp"
#include "common/env_util.hpp"
#include "common/logger.hpp"
#include "common/string_util.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string_view>
#include <thread>
#include <vector>

std::uint32_t default_http_worker_count(){
    const std::uint32_t hardware_thread_count = std::thread::hardware_concurrency();
    return hardware_thread_count == 0 ? std::uint32_t{1} : hardware_thread_count;
}

std::expected<std::uint32_t, error_code> resolve_http_worker_count(){
    const char* worker_count_text = std::getenv("HTTP_WORKER_COUNT");
    if(worker_count_text == nullptr || *worker_count_text == '\0'){
        return default_http_worker_count();
    }

    const auto worker_count_opt = string_util::parse_positive_int64(worker_count_text);
    if(
        !worker_count_opt ||
        *worker_count_opt > static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max())
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return static_cast<std::uint32_t>(*worker_count_opt);
}

int main(){
    const auto require_http_envs_exp = env_util::require_http_server_envs();
    if(!require_http_envs_exp){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "required_env_missing");
        return 1;
    }

    const char* http_port_text = std::getenv("HTTP_PORT");
    if(http_port_text == nullptr || std::string_view{http_port_text}.empty()){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "http_port_missing");
        return 1;
    }

    const auto port_opt = string_util::parse_positive_int16(http_port_text);
    if(!port_opt){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "invalid_http_port")
            .field("http_port", http_port_text);
        return 1;
    }
    const std::uint16_t port = *port_opt;

    const auto worker_count_exp = resolve_http_worker_count();
    if(!worker_count_exp){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "invalid_http_worker_count");
        return 1;
    }
    const std::uint32_t worker_count = *worker_count_exp;

    boost::asio::io_context io_context{static_cast<int>(worker_count)};
    auto http_server_exp = http_server::create(static_cast<std::size_t>(worker_count));
    if(!http_server_exp){
        logger::cerr()
            .log("http_server_create_failed")
            .field("error", to_string(http_server_exp.error()));
        return 1;
    }

    auto acceptor_exp = acceptor::create(
        io_context,
        acceptor::tcp::endpoint{acceptor::tcp::v4(), port},
        *http_server_exp
    );
    
    if(!acceptor_exp){
        logger::cerr()
            .log("http_acceptor_create_failed")
            .field("error", to_string(acceptor_exp.error()))
            .field("port", port);
        return 1;
    }

    auto run_exp = (*acceptor_exp)->run();
    if(!run_exp){
        logger::cerr()
            .log("http_acceptor_run_failed")
            .field("error", to_string(run_exp.error()))
            .field("port", port);
        return 1;
    }

    logger::cerr()
        .log("http_server_start")
        .field("port", port)
        .field("worker_count", worker_count);

    std::vector<std::thread> worker_threads;
    worker_threads.reserve(worker_count > 0 ? worker_count - 1 : 0);
    for(std::uint32_t worker_index = 1; worker_index < worker_count; ++worker_index){
        worker_threads.emplace_back(
            [&io_context]{
                io_context.run();
            }
        );
    }

    io_context.run();

    for(auto& worker_thread : worker_threads){
        worker_thread.join();
    }
    return 0;
}
