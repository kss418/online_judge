#pragma once

#include "dto/submission_dto.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

#include <cstdint>

namespace finalize_submission_mapper{
    submission_dto::finalize_request make_finalize_request(
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    submission_dto::finalize_request make_infra_failure_finalize_request(
        std::int64_t submission_id,
        std::string reason
    );
}
