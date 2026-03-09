#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"
#include "judge_server/code_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(submission_service submission_service);

    std::expected<void, error_code> run();
    std::expected<std::optional<queued_submission>, error_code> save_source_code();

private:
    judge_worker(submission_service submission_service, code_runner runner);

    static bool is_queue_empty_error(const error_code& code);
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};

    submission_service submission_service_;
    code_runner code_runner_;
};
