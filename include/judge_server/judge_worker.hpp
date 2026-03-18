#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "db/submission_service.hpp"
#include "db/submission_util.hpp"
#include "judge_server/testcase_downloader.hpp"
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
    static std::expected<judge_worker, error_code> create(submission_service submission_service);

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
        submission_service submission_service,
        db_connection db_connection,
        tc_downloader tc_downloader
    );

    static bool is_queue_empty_error(const error_code& code);
    static submission_status to_submission_status(judge_result result);
    static finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<judge_result, error_code> judge_submission(
        const submission_dto::queued_submission& queued_submission_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    static constexpr std::chrono::seconds lease_duration_{900};
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};

    submission_service submission_service_;
    db_connection db_connection_;
    tc_downloader tc_downloader_;
};
