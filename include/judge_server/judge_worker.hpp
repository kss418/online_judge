#pragma once

#include "common/submission_status.hpp"
#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "db_event/submission_event_listener.hpp"
#include "dto/submission_dto.hpp"
#include "judge_server/testcase_downloader.hpp"
#include "judge_server/testcase_runner.hpp"
#include "judge_server/judge_util.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(
        submission_event_listener submission_event_listener
    );

    std::expected<void, error_code> run();
    std::expected<std::optional<submission_dto::queued_submission>, error_code> lease_submission();
    std::expected<void, error_code> save_source_code(
        const submission_dto::queued_submission& queued_submission_value
    );

private:
    struct finalize_submission_data{
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
    };

    judge_worker(
        submission_event_listener submission_event_listener,
        db_connection db_connection,
        testcase_downloader testcase_downloader
    );

    static submission_status to_submission_status(judge_result result);
    static finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<judge_result, error_code> judge_submission(
        std::int64_t problem_id,
        const testcase_runner::run_batch& run_batch_value
    );
    static constexpr std::chrono::seconds lease_duration_{900};
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};

    submission_event_listener submission_event_listener_;
    db_connection db_connection_;
    testcase_downloader testcase_downloader_;
};
