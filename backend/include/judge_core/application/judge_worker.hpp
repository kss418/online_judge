#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_processor.hpp"
#include "judge_core/application/submission_execution_service.hpp"
#include "judge_core/infrastructure/submission_queue_source.hpp"

#include <chrono>
#include <expected>
#include <filesystem>

class judge_worker{
public:
    struct dependencies{
        submission_queue_source submission_queue_source_value;
        db_connection submission_db_connection;
        db_connection testcase_snapshot_connection;
        submission_execution_service submission_execution_service_value;
        std::filesystem::path source_root_path;
    };

    static std::expected<judge_worker, judge_error> create(
        dependencies dependencies_value
    );

    std::expected<void, judge_error> run();

private:
    judge_worker(
        submission_queue_source submission_queue_source,
        db_connection submission_db_connection,
        submission_processor submission_processor_value
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_queue_source submission_queue_source_;
    db_connection db_connection_;
    submission_processor submission_processor_;
};
