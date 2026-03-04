#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

enum class submission_status{
    queued,
    judging,
    accepted,
    wrong_answer,
    time_limit_exceeded,
    memory_limit_exceeded,
    runtime_error,
    compile_error,
    output_exceeded
};

std::string to_string(submission_status status);

struct queued_submission{
    std::int64_t submission_id = 0;
    std::int64_t problem_id = 0;
    std::string language;
    std::string source_code;
};

class submission_service : public db_service_base<submission_service>{
public:
    std::expected<std::int64_t, error_code> create_submission(
        std::int64_t user_id,
        std::int64_t problem_id,
        const std::string& language,
        const std::string& source_code
    );

    std::expected<void, error_code> update_submission_status(
        std::int64_t submission_id,
        submission_status to_status,
        const std::optional<std::string>& reason = std::nullopt
    );

    std::expected<void, error_code> listen_submission_queue();
    std::expected<bool, error_code> wait_submission_notification(std::chrono::milliseconds timeout);
    std::expected<queued_submission, error_code> pop_submission();
    std::expected<queued_submission, error_code> lease_submission(std::chrono::seconds lease_duration);
    std::expected<void, error_code> finalize_submission(
        std::int64_t submission_id,
        submission_status to_status,
        std::optional<std::int16_t> score,
        std::optional<std::string> compile_output,
        std::optional<std::string> judge_output,
        std::optional<std::string> reason
    );
private:
    friend class db_service_base<submission_service>;

    explicit submission_service(db_connection connection);

    static constexpr std::string_view submission_queue_channel_ = "submission_queue";
};
