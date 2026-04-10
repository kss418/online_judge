#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/infrastructure/judge_runtime_registry.hpp"

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class judge_runtime_status_reporter{
public:
    static std::expected<judge_runtime_status_reporter, judge_error> create(
        const db_connection_config& db_config,
        std::string instance_id,
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry
    );

    judge_runtime_status_reporter(judge_runtime_status_reporter&& other) noexcept;
    judge_runtime_status_reporter& operator=(judge_runtime_status_reporter&& other) noexcept;
    ~judge_runtime_status_reporter();

    judge_runtime_status_reporter(const judge_runtime_status_reporter&) = delete;
    judge_runtime_status_reporter& operator=(const judge_runtime_status_reporter&) = delete;

    std::expected<void, judge_error> publish_heartbeat();

    std::expected<void, judge_error> publish_self_check_result(
        std::string_view status,
        std::optional<std::string> message_opt = std::nullopt
    );

    const std::string& instance_id() const noexcept{
        return instance_id_;
    }

private:
    judge_runtime_status_reporter(
        db_connection db_connection_value,
        std::string instance_id,
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry
    );

    db_connection db_connection_;
    std::string instance_id_;
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry_;
};
