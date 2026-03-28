#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace pqxx{
    class row;
}

namespace user_statistics_dto{
    struct submission_statistics{
        std::int64_t user_id = 0;
        std::int64_t submission_count = 0;
        std::int64_t queued_submission_count = 0;
        std::int64_t judging_submission_count = 0;
        std::int64_t accepted_submission_count = 0;
        std::int64_t wrong_answer_submission_count = 0;
        std::int64_t time_limit_exceeded_submission_count = 0;
        std::int64_t memory_limit_exceeded_submission_count = 0;
        std::int64_t runtime_error_submission_count = 0;
        std::int64_t compile_error_submission_count = 0;
        std::int64_t output_exceeded_submission_count = 0;
        std::optional<std::string> last_submission_at_opt = std::nullopt;
        std::optional<std::string> last_accepted_at_opt = std::nullopt;
        std::string updated_at;
    };

    submission_statistics make_submission_statistics_from_row(
        const pqxx::row& user_submission_statistics_row
    );
}
