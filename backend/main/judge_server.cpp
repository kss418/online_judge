#include "common/env_util.hpp"
#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "common/logger.hpp"
#include "common/string_util.hpp"
#include "db_event/submission_event_listener.hpp"
#include "judge_core/problem_lock_registry.hpp"
#include "judge_core/judge_worker.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

constexpr std::chrono::seconds WORKER_RESTART_DELAY{1};

std::expected<submission_event_listener, error_code> create_submission_event_listener(){
    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    auto submission_event_listener_exp = submission_event_listener::create(
        std::move(*db_connection_exp)
    );
    if(!submission_event_listener_exp){
        return std::unexpected(submission_event_listener_exp.error());
    }

    return std::move(*submission_event_listener_exp);
}

std::uint32_t default_worker_count(){
    const std::uint32_t hardware_thread_count = std::thread::hardware_concurrency();
    return hardware_thread_count == 0 ? std::uint32_t{1} : hardware_thread_count;
}

std::expected<std::uint32_t, error_code> resolve_worker_count(){
    const char* worker_count_text = std::getenv("JUDGE_WORKER_COUNT");
    if(worker_count_text == nullptr || *worker_count_text == '\0'){
        return default_worker_count();
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

void run_judge_worker_loop(
    std::uint32_t worker_index,
    const std::shared_ptr<problem_lock_registry>& shared_problem_lock_registry
){
    while(true){
        auto submission_event_listener_exp = create_submission_event_listener();
        if(!submission_event_listener_exp){
            logger::cerr()
                .log("judge_worker_initialize_failed")
                .field("worker_index", worker_index)
                .field("stage", "submission_event_listener")
                .field("error", to_string(submission_event_listener_exp.error()));
            std::this_thread::sleep_for(WORKER_RESTART_DELAY);
            continue;
        }

        auto judge_worker_exp = judge_worker::create(
            std::move(*submission_event_listener_exp),
            shared_problem_lock_registry
        );
        if(!judge_worker_exp){
            logger::cerr()
                .log("judge_worker_initialize_failed")
                .field("worker_index", worker_index)
                .field("stage", "judge_worker")
                .field("error", to_string(judge_worker_exp.error()));
            std::this_thread::sleep_for(WORKER_RESTART_DELAY);
            continue;
        }

        auto run_exp = judge_worker_exp->run();
        if(run_exp){
            logger::cerr()
                .log("judge_worker_stopped")
                .field("worker_index", worker_index)
                .field("result", "unexpected_stop")
                .field("action", "restart");
        }
        else{
            logger::cerr()
                .log("judge_worker_run_failed")
                .field("worker_index", worker_index)
                .field("error", to_string(run_exp.error()))
                .field("action", "restart");
        }

        std::this_thread::sleep_for(WORKER_RESTART_DELAY);
    }
}

int main(){
    const auto require_judge_envs_exp = env_util::require_judge_server_envs();
    if(!require_judge_envs_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "required_env_missing");
        return 1;
    }

    const auto sandbox_self_check_exp = sandbox_runner::startup_self_check();
    if(!sandbox_self_check_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "sandbox_startup_self_check_failed")
            .field("error", to_string(sandbox_self_check_exp.error()));
        return 1;
    }

    const auto worker_count_exp = resolve_worker_count();
    if(!worker_count_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "invalid_judge_worker_count");
        return 1;
    }
    const std::uint32_t worker_count = *worker_count_exp;

    auto shared_problem_lock_registry = std::make_shared<problem_lock_registry>();
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(worker_count);

    logger::cerr()
        .log("judge_server_start")
        .field("worker_count", worker_count);

    for(std::uint32_t worker_index = 1; worker_index <= worker_count; ++worker_index){
        worker_threads.emplace_back(
            [worker_index, shared_problem_lock_registry]{
                run_judge_worker_loop(worker_index, shared_problem_lock_registry);
            }
        );
    }

    for(auto& worker_thread : worker_threads){
        worker_thread.join();
    }

    return 0;
}
