#pragma once

#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/execution_report.hpp"

namespace compile_failure_report_mapper{
    execution_report::batch make_execution_report(
        const compile_failure& compile_failure_value
    );
}
