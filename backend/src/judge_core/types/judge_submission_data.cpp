#include "judge_core/types/judge_submission_data.hpp"

#include <utility>

namespace judge_submission_data{
    process_submission_data make_process_submission_data(
        judge_result judge_result_value,
        execution_report::batch&& execution_report_value
    ){
        process_submission_data process_submission_data_value;
        process_submission_data_value.judge_result_value = judge_result_value;
        process_submission_data_value.execution_report_value =
            std::move(execution_report_value);
        return process_submission_data_value;
    }
}
