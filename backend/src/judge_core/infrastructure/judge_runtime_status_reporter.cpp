#include "judge_core/infrastructure/judge_runtime_status_reporter.hpp"

#include "db_service/db_service_util.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    std::expected<void, service_error> upsert_judge_instance_heartbeat(
        pqxx::work& transaction,
        std::string_view instance_id,
        const judge_runtime_registry& judge_runtime_registry
    ){
        transaction.exec(
            "INSERT INTO judge_instances("
            "    instance_id, "
            "    started_at, "
            "    last_heartbeat_at, "
            "    configured_worker_count, "
            "    busy_worker_count, "
            "    snapshot_cache_hit_count, "
            "    snapshot_cache_miss_count"
            ") "
            "VALUES($1, NOW(), NOW(), $2, $3, $4, $5) "
            "ON CONFLICT(instance_id) DO UPDATE SET "
            "    last_heartbeat_at = NOW(), "
            "    configured_worker_count = EXCLUDED.configured_worker_count, "
            "    busy_worker_count = EXCLUDED.busy_worker_count, "
            "    snapshot_cache_hit_count = EXCLUDED.snapshot_cache_hit_count, "
            "    snapshot_cache_miss_count = EXCLUDED.snapshot_cache_miss_count",
            pqxx::params{
                instance_id,
                judge_runtime_registry.configured_worker_count(),
                judge_runtime_registry.busy_worker_count(),
                judge_runtime_registry.snapshot_cache_hit_count(),
                judge_runtime_registry.snapshot_cache_miss_count()
            }
        );
        return {};
    }

    std::expected<void, service_error> upsert_judge_instance_self_check(
        pqxx::work& transaction,
        std::string_view instance_id,
        const judge_runtime_registry& judge_runtime_registry,
        std::string_view status,
        const std::optional<std::string>& message_opt
    ){
        transaction.exec(
            "INSERT INTO judge_instances("
            "    instance_id, "
            "    started_at, "
            "    last_heartbeat_at, "
            "    configured_worker_count, "
            "    busy_worker_count, "
            "    snapshot_cache_hit_count, "
            "    snapshot_cache_miss_count, "
            "    last_sandbox_self_check_status, "
            "    last_sandbox_self_check_at, "
            "    last_sandbox_self_check_message"
            ") "
            "VALUES($1, NOW(), NOW(), $2, $3, $4, $5, $6, NOW(), $7) "
            "ON CONFLICT(instance_id) DO UPDATE SET "
            "    last_heartbeat_at = NOW(), "
            "    configured_worker_count = EXCLUDED.configured_worker_count, "
            "    busy_worker_count = EXCLUDED.busy_worker_count, "
            "    snapshot_cache_hit_count = EXCLUDED.snapshot_cache_hit_count, "
            "    snapshot_cache_miss_count = EXCLUDED.snapshot_cache_miss_count, "
            "    last_sandbox_self_check_status = EXCLUDED.last_sandbox_self_check_status, "
            "    last_sandbox_self_check_at = NOW(), "
            "    last_sandbox_self_check_message = EXCLUDED.last_sandbox_self_check_message",
            pqxx::params{
                instance_id,
                judge_runtime_registry.configured_worker_count(),
                judge_runtime_registry.busy_worker_count(),
                judge_runtime_registry.snapshot_cache_hit_count(),
                judge_runtime_registry.snapshot_cache_miss_count(),
                status,
                message_opt
            }
        );
        return {};
    }
}

std::expected<judge_runtime_status_reporter, judge_error>
judge_runtime_status_reporter::create(
    const db_connection_config& db_config,
    std::string instance_id,
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry
){
    if(instance_id.empty() || !judge_runtime_registry){
        return std::unexpected(judge_error::validation_error);
    }

    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return judge_runtime_status_reporter(
        std::move(*db_connection_exp),
        std::move(instance_id),
        std::move(judge_runtime_registry)
    );
}

judge_runtime_status_reporter::judge_runtime_status_reporter(
    db_connection db_connection_value,
    std::string instance_id,
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry
) :
    db_connection_(std::move(db_connection_value)),
    instance_id_(std::move(instance_id)),
    judge_runtime_registry_(std::move(judge_runtime_registry)){}

judge_runtime_status_reporter::judge_runtime_status_reporter(
    judge_runtime_status_reporter&& other
) noexcept = default;

judge_runtime_status_reporter& judge_runtime_status_reporter::operator=(
    judge_runtime_status_reporter&& other
) noexcept = default;

judge_runtime_status_reporter::~judge_runtime_status_reporter() = default;

std::expected<void, judge_error> judge_runtime_status_reporter::publish_heartbeat(){
    const auto publish_exp = db_service_util::with_retry_service_write_transaction(
        db_connection_,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            return upsert_judge_instance_heartbeat(
                transaction,
                instance_id_,
                *judge_runtime_registry_
            );
        }
    );
    if(!publish_exp){
        return std::unexpected(judge_error{publish_exp.error()});
    }

    return {};
}

std::expected<void, judge_error> judge_runtime_status_reporter::publish_self_check_result(
    std::string_view status,
    std::optional<std::string> message_opt
){
    const auto publish_exp = db_service_util::with_retry_service_write_transaction(
        db_connection_,
        [&](pqxx::work& transaction) -> std::expected<void, service_error> {
            return upsert_judge_instance_self_check(
                transaction,
                instance_id_,
                *judge_runtime_registry_,
                status,
                message_opt
            );
        }
    );
    if(!publish_exp){
        return std::unexpected(judge_error{publish_exp.error()});
    }

    return {};
}
