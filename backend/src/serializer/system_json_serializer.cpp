#include "serializer/system_json_serializer.hpp"

#include <cstdint>

namespace{
    boost::json::object make_supported_language_object(
        const language_util::supported_language& supported_language_value
    ){
        boost::json::object response_object;
        response_object["language"] = supported_language_value.language;
        response_object["source_extension"] = supported_language_value.source_extension;
        return response_object;
    }

    boost::json::array make_supported_language_array(
        std::span<const language_util::supported_language> supported_language_values
    ){
        boost::json::array response_array;
        response_array.reserve(supported_language_values.size());
        for(const auto& supported_language_value : supported_language_values){
            response_array.push_back(make_supported_language_object(supported_language_value));
        }
        return response_array;
    }

    boost::json::object make_http_runtime_object(
        const system_dto::http_runtime_snapshot& http_runtime_value
    ){
        boost::json::object db_pool_object;
        db_pool_object["size"] = http_runtime_value.db_pool.size;
        db_pool_object["available_count"] = http_runtime_value.db_pool.available_count;

        boost::json::object handler_queue_object;
        handler_queue_object["backlog_count"] =
            http_runtime_value.handler_queue.backlog_count;
        if(http_runtime_value.handler_queue.limit_opt.has_value()){
            handler_queue_object["limit"] = *http_runtime_value.handler_queue.limit_opt;
        }
        else{
            handler_queue_object["limit"] = nullptr;
        }

        boost::json::object http_object;
        http_object["db_pool"] = std::move(db_pool_object);
        http_object["handler_queue"] = std::move(handler_queue_object);
        return http_object;
    }

    boost::json::object make_snapshot_cache_object(
        const system_dto::snapshot_cache_metrics& snapshot_cache_value
    ){
        boost::json::object snapshot_cache_object;
        snapshot_cache_object["hit_count"] = snapshot_cache_value.hit_count;
        snapshot_cache_object["miss_count"] = snapshot_cache_value.miss_count;
        return snapshot_cache_object;
    }

    boost::json::object make_sandbox_self_check_object(
        const system_dto::sandbox_self_check& self_check_value
    ){
        boost::json::object self_check_object;
        self_check_object["status"] = self_check_value.status;
        if(self_check_value.checked_at_opt.has_value()){
            self_check_object["checked_at"] = *self_check_value.checked_at_opt;
        }
        else{
            self_check_object["checked_at"] = nullptr;
        }

        if(self_check_value.message_opt.has_value()){
            self_check_object["message"] = *self_check_value.message_opt;
        }
        else{
            self_check_object["message"] = nullptr;
        }
        return self_check_object;
    }

    boost::json::object make_judge_status_object(
        const system_dto::judge_status_snapshot& judge_status_value
    ){
        boost::json::object judge_object;
        judge_object["queue_depth"] = judge_status_value.queue_depth;
        judge_object["live_instance_count"] = judge_status_value.live_instance_count;
        judge_object["configured_worker_count"] =
            judge_status_value.configured_worker_count;
        judge_object["active_worker_count"] = judge_status_value.active_worker_count;
        judge_object["snapshot_cache"] = make_snapshot_cache_object(
            judge_status_value.snapshot_cache
        );
        judge_object["last_sandbox_self_check"] = make_sandbox_self_check_object(
            judge_status_value.last_sandbox_self_check
        );
        return judge_object;
    }
}

boost::json::object system_json_serializer::make_supported_language_list_object(
    std::span<const language_util::supported_language> supported_language_values
){
    boost::json::object response_object;
    response_object["language_count"] =
        static_cast<std::int64_t>(supported_language_values.size());
    response_object["languages"] = make_supported_language_array(supported_language_values);
    return response_object;
}

boost::json::object system_json_serializer::make_status_object(
    const system_dto::status_response& status_value
){
    boost::json::object response_object;
    response_object["http"] = make_http_runtime_object(status_value.http);
    response_object["judge"] = make_judge_status_object(status_value.judge);
    return response_object;
}
