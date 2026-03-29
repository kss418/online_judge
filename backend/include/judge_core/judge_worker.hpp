#pragma once

#include "common/submission_status.hpp"
#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "db_event/submission_event_listener.hpp"
#include "dto/submission_dto.hpp"
#include "judge_core/problem_lock_registry.hpp"
#include "judge_core/testcase_downloader.hpp"
#include "judge_core/testcase_runner.hpp"
#include "judge_core/judge_util.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(
        submission_event_listener submission_event_listener,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    std::expected<void, error_code> run();
    std::expected<std::optional<submission_dto::queued_submission>, error_code> lease_submission();

private:
    struct finalize_submission_data{
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
    };
    struct process_submission_data{
        judge_result judge_result_value = judge_result::wrong_answer;
        std::vector<sandbox_runner::run_result> run_results;
    };

    judge_worker(
        submission_event_listener submission_event_listener,
        db_connection db_connection,
        testcase_downloader testcase_downloader,
        std::shared_ptr<problem_lock_registry> problem_lock_registry
    );

    static submission_status to_submission_status(judge_result result);
    static finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<void, error_code> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<void, error_code> process_submission(
        const submission_dto::queued_submission& queued_submission_value
    );
    std::expected<std::filesystem::path, error_code> prepare_submission(
        const submission_dto::queued_submission& queued_submission_value
    );
    std::expected<void, error_code> cleanup_submission_workspace(std::int64_t submission_id);
    std::expected<void, error_code> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );
    std::expected<process_submission_data, error_code> judge_submission(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );
    std::expected<judge_result, error_code> check_result(
        std::int64_t problem_id,
        const testcase_runner::run_batch& run_batch_value
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_event_listener submission_event_listener_;
    db_connection db_connection_;
    testcase_downloader testcase_downloader_;
    std::shared_ptr<problem_lock_registry> problem_lock_registry_;
};
