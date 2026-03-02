#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"

#include <chrono>
#include <expected>
#include <filesystem>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(submission_service submission_service);

    std::expected<void, error_code> run();
    std::expected<bool, error_code> save_source_code();

private:
    explicit judge_worker(
        submission_service submission_service,
        std::filesystem::path source_root_path
    );

    static bool is_queue_empty_error(const error_code& code);
    static constexpr std::chrono::milliseconds notification_wait_timeout_{30000};

    submission_service submission_service_;
    std::filesystem::path source_root_path_;
};
