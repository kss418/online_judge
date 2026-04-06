#include "common/db_connection.hpp"
#include "common/env_util.hpp"
#include "common/file_util.hpp"
#include "common/logger.hpp"
#include "common/string_util.hpp"
#include "error/infra_error.hpp"
#include "judge_core/application/execution_engine.hpp"
#include "judge_core/application/judge_evaluator.hpp"
#include "judge_core/entry/judge_worker.hpp"
#include "judge_core/entry/submission_processor.hpp"
#include "judge_core/gateway/judge_queue_port.hpp"
#include "judge_core/gateway/judge_submission_port.hpp"
#include "judge_core/gateway/testcase_snapshot_port.hpp"
#include "judge_core/infrastructure/problem_lock_registry.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

constexpr std::chrono::seconds WORKER_RESTART_DELAY{1};

std::expected<std::filesystem::path, judge_error> load_required_path_env(
    const char* key,
    std::string empty_value_message
){
    const auto path_text_exp = env_util::require_env(key);
    if(!path_text_exp){
        return std::unexpected(path_text_exp.error());
    }

    const std::filesystem::path path_value(*path_text_exp);
    if(path_value.empty()){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                std::move(empty_value_message)
            }
        );
    }

    return path_value;
}

std::expected<judge_worker::dependencies, judge_error> build_judge_worker_dependencies(
    const std::shared_ptr<problem_lock_registry>& shared_problem_lock_registry
){
    const auto source_root_path_exp = load_required_path_env(
        "JUDGE_SOURCE_ROOT",
        "judge source root is not configured"
    );
    if(!source_root_path_exp){
        return std::unexpected(source_root_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        *source_root_path_exp
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto testcase_root_path_exp = load_required_path_env(
        "TESTCASE_PATH",
        "testcase root path is not configured"
    );
    if(!testcase_root_path_exp){
        return std::unexpected(testcase_root_path_exp.error());
    }

    auto db_config_exp = db_connection::load_db_connection_config();
    if(!db_config_exp){
        return std::unexpected(db_config_exp.error());
    }

    auto judge_queue_port_exp = judge_queue_port::create(*db_config_exp);
    if(!judge_queue_port_exp){
        return std::unexpected(judge_queue_port_exp.error());
    }

    auto testcase_snapshot_port_exp = testcase_snapshot_port::create(
        *db_config_exp,
        *testcase_root_path_exp,
        shared_problem_lock_registry
    );
    if(!testcase_snapshot_port_exp){
        return std::unexpected(testcase_snapshot_port_exp.error());
    }

    auto execution_engine_exp = execution_engine::create(
        std::move(*testcase_snapshot_port_exp)
    );
    if(!execution_engine_exp){
        return std::unexpected(execution_engine_exp.error());
    }

    auto judge_evaluator_exp = judge_evaluator::create();
    if(!judge_evaluator_exp){
        return std::unexpected(judge_evaluator_exp.error());
    }

    auto judge_submission_port_exp = judge_submission_port::create(*db_config_exp);
    if(!judge_submission_port_exp){
        return std::unexpected(judge_submission_port_exp.error());
    }

    auto submission_processor_exp = submission_processor::create(
        submission_processor::dependencies{
            .judge_queue_port_value = std::move(*judge_queue_port_exp),
            .judge_submission_port_value = std::move(*judge_submission_port_exp),
            .execution_engine_value = std::move(*execution_engine_exp),
            .judge_evaluator_value = std::move(*judge_evaluator_exp),
            .source_root_path = std::move(*source_root_path_exp),
        }
    );
    if(!submission_processor_exp){
        return std::unexpected(submission_processor_exp.error());
    }

    return judge_worker::dependencies{
        .submission_processor_value = std::move(*submission_processor_exp),
    };
}

std::uint32_t default_worker_count(){
    const std::uint32_t hardware_thread_count = std::thread::hardware_concurrency();
    return hardware_thread_count == 0 ? std::uint32_t{1} : hardware_thread_count;
}

std::expected<std::uint32_t, infra_error> resolve_worker_count(){
    const char* worker_count_text = std::getenv("JUDGE_WORKER_COUNT");
    if(worker_count_text == nullptr || *worker_count_text == '\0'){
        return default_worker_count();
    }

    const auto worker_count_opt = string_util::parse_positive_int64(worker_count_text);
    if(
        !worker_count_opt ||
        *worker_count_opt > static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max())
    ){
        return std::unexpected(infra_error::invalid_argument);
    }

    return static_cast<std::uint32_t>(*worker_count_opt);
}

void run_judge_worker_loop(
    std::uint32_t worker_index,
    const std::shared_ptr<problem_lock_registry>& shared_problem_lock_registry
){
    while(true){
        auto judge_worker_dependencies_exp = build_judge_worker_dependencies(
            shared_problem_lock_registry
        );
        if(!judge_worker_dependencies_exp){
            logger::cerr()
                .log("judge_worker_initialize_failed")
                .field("worker_index", worker_index)
                .field("stage", "judge_worker_dependencies")
                .field("error", to_string(judge_worker_dependencies_exp.error()));
            std::this_thread::sleep_for(WORKER_RESTART_DELAY);
            continue;
        }

        auto judge_worker_exp = judge_worker::create(
            std::move(*judge_worker_dependencies_exp)
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

        sandbox_runner::invalidate_cached_artifacts();
        logger::cerr()
            .log("sandbox_artifact_cache_invalidated")
            .field("scope", "all")
            .field("reason", "worker_restart");

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
