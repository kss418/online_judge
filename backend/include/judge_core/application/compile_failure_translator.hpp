#pragma once

#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/execution_report.hpp"

class compile_failure_translator{
public:
    execution_report::batch translate(
        const compile_failure& compile_failure_value
    ) const;
};
