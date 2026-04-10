#include "db_service/system_service.hpp"

#include "db_service/db_service_util.hpp"

#include <pqxx/pqxx>

namespace{
    std::expected<std::int64_t, service_error> get_queue_depth(
        pqxx::read_transaction& transaction
    ){
        const auto result = transaction.exec(
            "SELECT COUNT(*)::bigint "
            "FROM submission_queue "
            "WHERE leased_until IS NULL OR leased_until <= NOW()"
        );
        return result[0][0].as<std::int64_t>();
    }

    std::expected<void, service_error> fill_live_judge_aggregate(
        pqxx::read_transaction& transaction,
        std::chrono::milliseconds judge_heartbeat_stale_after,
        system_dto::judge_status_snapshot& judge_status_value
    ){
        const auto live_result = transaction.exec(
            "SELECT "
            "COUNT(*)::bigint AS live_instance_count, "
            "COALESCE(SUM(configured_worker_count), 0)::bigint "
            "    AS configured_worker_count, "
            "COALESCE(SUM(busy_worker_count), 0)::bigint "
            "    AS active_worker_count, "
            "COALESCE(SUM(snapshot_cache_hit_count), 0)::bigint "
            "    AS snapshot_cache_hit_count, "
            "COALESCE(SUM(snapshot_cache_miss_count), 0)::bigint "
            "    AS snapshot_cache_miss_count "
            "FROM judge_instances "
            "WHERE last_heartbeat_at >= NOW() - ($1 * INTERVAL '1 millisecond')",
            pqxx::params{judge_heartbeat_stale_after.count()}
        );

        judge_status_value.live_instance_count = live_result[0][0].as<std::int64_t>();
        judge_status_value.configured_worker_count =
            live_result[0][1].as<std::int64_t>();
        judge_status_value.active_worker_count =
            live_result[0][2].as<std::int64_t>();
        judge_status_value.snapshot_cache.hit_count =
            live_result[0][3].as<std::int64_t>();
        judge_status_value.snapshot_cache.miss_count =
            live_result[0][4].as<std::int64_t>();
        return {};
    }

    std::expected<void, service_error> fill_last_sandbox_self_check(
        pqxx::read_transaction& transaction,
        system_dto::judge_status_snapshot& judge_status_value
    ){
        const auto self_check_result = transaction.exec(
            "SELECT "
            "last_sandbox_self_check_status, "
            "last_sandbox_self_check_at::text, "
            "last_sandbox_self_check_message "
            "FROM judge_instances "
            "WHERE last_sandbox_self_check_at IS NOT NULL "
            "ORDER BY last_sandbox_self_check_at DESC "
            "LIMIT 1"
        );

        if(self_check_result.empty()){
            judge_status_value.last_sandbox_self_check.status = "unknown";
            judge_status_value.last_sandbox_self_check.message_opt =
                "judge has not reported self-check yet";
            return {};
        }

        judge_status_value.last_sandbox_self_check.status =
            self_check_result[0][0].as<std::string>();
        judge_status_value.last_sandbox_self_check.checked_at_opt =
            self_check_result[0][1].as<std::string>();
        if(!self_check_result[0][2].is_null()){
            judge_status_value.last_sandbox_self_check.message_opt =
                self_check_result[0][2].as<std::string>();
        }

        return {};
    }
}

std::expected<system_dto::status_response, service_error> system_service::get_status(
    db_connection& connection,
    const system_dto::http_runtime_snapshot& http_runtime_snapshot,
    std::chrono::milliseconds judge_heartbeat_stale_after
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<system_dto::status_response, service_error> {
            system_dto::status_response status_value;
            status_value.http = http_runtime_snapshot;

            const auto queue_depth_exp = get_queue_depth(transaction);
            if(!queue_depth_exp){
                return std::unexpected(queue_depth_exp.error());
            }
            status_value.judge.queue_depth = *queue_depth_exp;

            const auto live_aggregate_exp = fill_live_judge_aggregate(
                transaction,
                judge_heartbeat_stale_after,
                status_value.judge
            );
            if(!live_aggregate_exp){
                return std::unexpected(live_aggregate_exp.error());
            }

            const auto self_check_exp = fill_last_sandbox_self_check(
                transaction,
                status_value.judge
            );
            if(!self_check_exp){
                return std::unexpected(self_check_exp.error());
            }

            return status_value;
        }
    );
}
