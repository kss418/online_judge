#include "common/env_util.hpp"
#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "db_event/submission_event_listener.hpp"
#include "judge_core/judge_worker.hpp"

#include <chrono>
#include <expected>
#include <iostream>
#include <thread>
#include <utility>

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

int main(){
    const auto require_all_envs_exp = env_util::require_all_envs();
    if(!require_all_envs_exp){
        std::cerr << "required environment variables are missing\n";
        return 1;
    }

    while(true){
        auto submission_event_listener_exp = create_submission_event_listener();
        if(!submission_event_listener_exp){
            std::cerr << "failed to initialize submission_event_listener: "
                      << to_string(submission_event_listener_exp.error()) << '\n';
            std::this_thread::sleep_for(WORKER_RESTART_DELAY);
            continue;
        }

        auto judge_worker_exp = judge_worker::create(
            std::move(*submission_event_listener_exp)
        );
        if(!judge_worker_exp){
            std::cerr << "failed to initialize judge_worker: "
                      << to_string(judge_worker_exp.error()) << '\n';
            std::this_thread::sleep_for(WORKER_RESTART_DELAY);
            continue;
        }

        auto run_exp = judge_worker_exp->run();
        if(run_exp){
            return 0;
        }

        std::cerr << "judge_worker run failed: "
                  << to_string(run_exp.error()) << '\n';
        std::this_thread::sleep_for(WORKER_RESTART_DELAY);
    }
}
