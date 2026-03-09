#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"
#include "judge_server/code_runner.hpp"
#include "judge_server/judge_utility.hpp"

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
    std::expected<std::optional<queued_submission>, error_code> save_source_code();

private:
    struct finalize_submission_data{
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
    };

    judge_worker(submission_service submission_service, code_runner runner);

    static bool is_queue_empty_error(const error_code& code);
    static submission_status to_submission_status(judge_result result);
    static finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    std::expected<judge_result, error_code> judge_submission(
        const queued_submission& queued_submission_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};

    submission_service submission_service_;
    code_runner code_runner_;
};
