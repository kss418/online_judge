#include "http_core/acceptor.hpp"
#include "http_core/http_runtime_config.hpp"
#include "http_core/http_server.hpp"
#include "common/env_util.hpp"
#include "common/logger.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdint>
#include <thread>
#include <vector>

std::uint32_t default_http_worker_count(){
    const std::uint32_t hardware_thread_count = std::thread::hardware_concurrency();
    return hardware_thread_count == 0 ? std::uint32_t{1} : hardware_thread_count;
}

int main(){
    const auto require_http_envs_exp = env_util::require_http_server_envs();
    if(!require_http_envs_exp){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "required_env_missing");
        return 1;
    }

    const auto runtime_config_exp = http_runtime_config::load(default_http_worker_count());
    if(!runtime_config_exp){
        logger::cerr()
            .log("http_server_startup_error")
            .field("reason", "invalid_http_runtime_config")
            .field("error", to_string(runtime_config_exp.error()));
        return 1;
    }
    const auto& runtime_config = *runtime_config_exp;

    boost::asio::io_context io_context{
        static_cast<int>(runtime_config.io_worker_count)
    };
    auto http_server_exp = http_server::create(runtime_config);
    if(!http_server_exp){
        logger::cerr()
            .log("http_server_create_failed")
            .field("error", to_string(http_server_exp.error()));
        return 1;
    }

    auto acceptor_exp = acceptor::create(
        io_context,
        acceptor::tcp::endpoint{acceptor::tcp::v4(), runtime_config.port},
        *http_server_exp
    );
    
    if(!acceptor_exp){
        logger::cerr()
            .log("http_acceptor_create_failed")
            .field("error", to_string(acceptor_exp.error()))
            .field("port", runtime_config.port);
        return 1;
    }

    auto run_exp = (*acceptor_exp)->run();
    if(!run_exp){
        logger::cerr()
            .log("http_acceptor_run_failed")
            .field("error", to_string(run_exp.error()))
            .field("port", runtime_config.port);
        return 1;
    }

    logger::cerr()
        .log("http_server_start")
        .field("port", runtime_config.port)
        .field("worker_count", runtime_config.io_worker_count)
        .field("handler_worker_count", runtime_config.handler_worker_count)
        .field("db_pool_size", runtime_config.db_pool_size)
        .field(
            "db_acquire_timeout_ms",
            runtime_config.db_acquire_timeout_opt.has_value()
                ? runtime_config.db_acquire_timeout_opt->count()
                : 0
        )
        .field(
            "handler_queue_limit",
            runtime_config.handler_queue_limit_opt.has_value()
                ? *runtime_config.handler_queue_limit_opt
                : 0
        )
        .field(
            "request_deadline_ms",
            runtime_config.request_deadline_opt.has_value()
                ? runtime_config.request_deadline_opt->count()
                : 0
        )
        .field(
            "keep_alive_idle_timeout_ms",
            runtime_config.keep_alive_idle_timeout_opt.has_value()
                ? runtime_config.keep_alive_idle_timeout_opt->count()
                : 0
        );

    std::vector<std::thread> worker_threads;
    worker_threads.reserve(
        runtime_config.io_worker_count > 0 ? runtime_config.io_worker_count - 1 : 0
    );
    for(
        std::size_t worker_index = 1;
        worker_index < runtime_config.io_worker_count;
        ++worker_index
    ){
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
