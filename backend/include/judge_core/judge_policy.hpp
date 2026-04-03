#pragma once

#include "common/submission_status.hpp"
#include "error/judge_error.hpp"
#include "judge_core/judge_workspace.hpp"
#include "judge_core/sandbox_runner.hpp"
#include "judge_core/testcase_runner.hpp"
#include "judge_core/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace judge_policy{
    struct finalize_submission_data{
        std::optional<std::int16_t> score = std::nullopt;
        std::optional<std::string> compile_output = std::nullopt;
        std::optional<std::string> judge_output = std::nullopt;
        std::optional<std::int64_t> elapsed_ms_opt = std::nullopt;
        std::optional<std::int64_t> max_rss_kb_opt = std::nullopt;
    };

    std::expected<judge_result, judge_error> check_result(
        const testcase_snapshot& testcase_snapshot_value,
        const testcase_runner::run_batch& run_batch_value
    );

    submission_status to_submission_status(judge_result result);

    finalize_submission_data make_finalize_submission_data(
        submission_status submission_status_value,
        const std::vector<sandbox_runner::run_result>& run_results
    );
}
