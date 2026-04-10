#include "common/db_connection.hpp"
#include "common/env_util.hpp"
#include "common/file_util.hpp"
#include "common/logger.hpp"
#include "common/string_util.hpp"
#include "common/token_util.hpp"
#include "error/infra_error.hpp"
#include "judge_core/application/judge_evaluator.hpp"
#include "judge_core/application/submission_builder.hpp"
#include "judge_core/entry/judge_worker.hpp"
#include "judge_core/entry/submission_processor.hpp"
#include "judge_core/gateway/judge_queue_facade.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"
#include "judge_core/gateway/testcase_snapshot_facade.hpp"
#include "judge_core/infrastructure/judge_runtime_registry.hpp"
#include "judge_core/infrastructure/judge_runtime_status_reporter.hpp"
#include "judge_core/infrastructure/problem_lock_registry.hpp"
#include "judge_core/infrastructure/program_executor.hpp"
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
    const std::shared_ptr<problem_lock_registry>& shared_problem_lock_registry,
    const std::shared_ptr<judge_runtime_registry>& shared_judge_runtime_registry
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

    auto judge_queue_facade_exp = judge_queue_facade::create(*db_config_exp);
    if(!judge_queue_facade_exp){
        return std::unexpected(judge_queue_facade_exp.error());
    }

    auto testcase_snapshot_facade_exp = testcase_snapshot_facade::create(
        *db_config_exp,
        *testcase_root_path_exp,
        shared_problem_lock_registry,
        shared_judge_runtime_registry
    );
    if(!testcase_snapshot_facade_exp){
        return std::unexpected(testcase_snapshot_facade_exp.error());
    }

    auto submission_builder_exp = submission_builder::create();
    if(!submission_builder_exp){
        return std::unexpected(submission_builder_exp.error());
    }

    auto program_executor_exp = program_executor::create();
    if(!program_executor_exp){
        return std::unexpected(program_executor_exp.error());
    }

    auto judge_evaluator_exp = judge_evaluator::create();
    if(!judge_evaluator_exp){
        return std::unexpected(judge_evaluator_exp.error());
    }

    auto judge_submission_facade_exp =
        judge_submission_facade::create(*db_config_exp);
    if(!judge_submission_facade_exp){
        return std::unexpected(judge_submission_facade_exp.error());
    }

    auto submission_processor_exp = submission_processor::create(
        submission_processor::dependencies{
            .judge_submission_facade_value =
                std::move(*judge_submission_facade_exp),
            .testcase_snapshot_facade_value =
                std::move(*testcase_snapshot_facade_exp),
            .submission_builder_value = std::move(*submission_builder_exp),
            .program_executor_value = std::move(*program_executor_exp),
            .judge_evaluator_value = std::move(*judge_evaluator_exp),
            .source_root_path = std::move(*source_root_path_exp),
        }
    );
    if(!submission_processor_exp){
        return std::unexpected(submission_processor_exp.error());
    }

    return judge_worker::dependencies{
        .judge_queue_facade_value = std::move(*judge_queue_facade_exp),
        .submission_processor_value = std::move(*submission_processor_exp),
        .judge_runtime_registry_value = shared_judge_runtime_registry,
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

std::expected<std::chrono::milliseconds, infra_error>
resolve_optional_positive_milliseconds_env(
    const char* key,
    std::chrono::milliseconds default_value
){
    const char* value = std::getenv(key);
    if(value == nullptr || *value == '\0'){
        return default_value;
    }

    const auto value_opt = string_util::parse_positive_int64(value);
    if(
        !value_opt ||
        *value_opt > static_cast<std::int64_t>(
            std::numeric_limits<std::int32_t>::max()
        )
    ){
        return std::unexpected(infra_error::invalid_argument);
    }

    return std::chrono::milliseconds{*value_opt};
}

std::expected<std::chrono::milliseconds, infra_error>
resolve_optional_nonnegative_milliseconds_env(
    const char* key,
    std::chrono::milliseconds default_value
){
    const char* value = std::getenv(key);
    if(value == nullptr || *value == '\0'){
        return default_value;
    }

    if(value[0] == '0' && value[1] == '\0'){
        return std::chrono::milliseconds{0};
    }

    const auto value_opt = string_util::parse_positive_int64(value);
    if(
        !value_opt ||
        *value_opt > static_cast<std::int64_t>(
            std::numeric_limits<std::int32_t>::max()
        )
    ){
        return std::unexpected(infra_error::invalid_argument);
    }

    return std::chrono::milliseconds{*value_opt};
}

std::expected<std::string, infra_error> make_judge_instance_id(){
    const auto worker_id_exp = env_util::require_env("WORKER_ID");
    if(!worker_id_exp){
        return std::unexpected(worker_id_exp.error());
    }

    const auto token_exp = token_util::generate_token();
    if(!token_exp){
        return std::unexpected(token_exp.error());
    }

    return std::string{*worker_id_exp} + "-" + *token_exp;
}

void run_judge_status_heartbeat_loop(
    std::stop_token stop_token,
    std::shared_ptr<judge_runtime_status_reporter> judge_runtime_status_reporter_value,
    std::chrono::milliseconds heartbeat_interval
){
    while(!stop_token.stop_requested()){
        std::this_thread::sleep_for(heartbeat_interval);
        if(stop_token.stop_requested()){
            return;
        }

        const auto publish_exp = judge_runtime_status_reporter_value->publish_heartbeat();
        if(!publish_exp){
            logger::cerr()
                .log("judge_status_heartbeat_publish_failed")
                .field(
                    "instance_id",
                    judge_runtime_status_reporter_value->instance_id()
                )
                .field("error", to_string(publish_exp.error()));
        }
    }
}

void run_judge_worker_loop(
    std::uint32_t worker_index,
    const std::shared_ptr<problem_lock_registry>& shared_problem_lock_registry,
    const std::shared_ptr<judge_runtime_registry>& shared_judge_runtime_registry
){
    while(true){
        auto judge_worker_dependencies_exp = build_judge_worker_dependencies(
            shared_problem_lock_registry,
            shared_judge_runtime_registry
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

    const auto worker_count_exp = resolve_worker_count();
    if(!worker_count_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "invalid_judge_worker_count");
        return 1;
    }
    const std::uint32_t worker_count = *worker_count_exp;

    const auto heartbeat_interval_exp = resolve_optional_positive_milliseconds_env(
        "JUDGE_HEARTBEAT_INTERVAL_MS",
        std::chrono::milliseconds{5000}
    );
    if(!heartbeat_interval_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "invalid_judge_heartbeat_interval");
        return 1;
    }
    const auto judge_heartbeat_interval = *heartbeat_interval_exp;

    const auto heartbeat_stale_after_exp = resolve_optional_nonnegative_milliseconds_env(
        "JUDGE_HEARTBEAT_STALE_AFTER_MS",
        std::chrono::milliseconds{15000}
    );
    if(!heartbeat_stale_after_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "invalid_judge_heartbeat_stale_after");
        return 1;
    }
    const auto judge_heartbeat_stale_after = *heartbeat_stale_after_exp;

    const auto instance_id_exp = make_judge_instance_id();
    if(!instance_id_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "invalid_judge_instance_id")
            .field("error", to_string(instance_id_exp.error()));
        return 1;
    }
    const auto& instance_id = *instance_id_exp;

    auto db_config_exp = db_connection::load_db_connection_config();
    if(!db_config_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "db_config_load_failed")
            .field("error", to_string(db_config_exp.error()));
        return 1;
    }

    auto shared_problem_lock_registry = std::make_shared<problem_lock_registry>();
    auto shared_judge_runtime_registry = std::make_shared<judge_runtime_registry>(
        static_cast<std::int64_t>(worker_count)
    );
    auto judge_runtime_status_reporter_exp =
        judge_runtime_status_reporter::create(
            *db_config_exp,
            instance_id,
            shared_judge_runtime_registry
        );
    if(!judge_runtime_status_reporter_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "judge_status_reporter_create_failed")
            .field("error", to_string(judge_runtime_status_reporter_exp.error()));
        return 1;
    }
    auto judge_runtime_status_reporter_value =
        std::make_shared<judge_runtime_status_reporter>(
            std::move(*judge_runtime_status_reporter_exp)
        );

    const auto sandbox_self_check_exp = sandbox_runner::startup_self_check();
    if(!sandbox_self_check_exp){
        const auto publish_self_check_exp =
            judge_runtime_status_reporter_value->publish_self_check_result(
                "failed",
                to_string(sandbox_self_check_exp.error())
            );
        if(!publish_self_check_exp){
            logger::cerr()
                .log("judge_self_check_status_publish_failed")
                .field("instance_id", instance_id)
                .field("error", to_string(publish_self_check_exp.error()));
        }

        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "sandbox_startup_self_check_failed")
            .field("error", to_string(sandbox_self_check_exp.error()));
        return 1;
    }

    const auto publish_self_check_exp =
        judge_runtime_status_reporter_value->publish_self_check_result("passed");
    if(!publish_self_check_exp){
        logger::cerr()
            .log("judge_server_startup_error")
            .field("reason", "judge_status_publish_failed")
            .field("error", to_string(publish_self_check_exp.error()));
        return 1;
    }

    std::vector<std::thread> worker_threads;
    worker_threads.reserve(worker_count);
    std::jthread heartbeat_thread(
        [judge_runtime_status_reporter_value, judge_heartbeat_interval](
            std::stop_token stop_token
        ){
            run_judge_status_heartbeat_loop(
                stop_token,
                judge_runtime_status_reporter_value,
                judge_heartbeat_interval
            );
        }
    );

    logger::cerr()
        .log("judge_server_start")
        .field("worker_count", worker_count)
        .field("instance_id", instance_id)
        .field("heartbeat_interval_ms", judge_heartbeat_interval.count())
        .field("heartbeat_stale_after_ms", judge_heartbeat_stale_after.count());

    for(std::uint32_t worker_index = 1; worker_index <= worker_count; ++worker_index){
        worker_threads.emplace_back(
            [worker_index, shared_problem_lock_registry, shared_judge_runtime_registry]{
                run_judge_worker_loop(
                    worker_index,
                    shared_problem_lock_registry,
                    shared_judge_runtime_registry
                );
            }
        );
    }

    for(auto& worker_thread : worker_threads){
        worker_thread.join();
    }

    return 0;
}
