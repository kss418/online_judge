#include "judge_core/judge_submission_data.hpp"

#include "judge_core/judge_policy.hpp"

#include <utility>

namespace judge_submission_data{
    submission_stage_metrics make_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value
    ){
        submission_stage_metrics submission_stage_metrics_value;
        submission_stage_metrics_value.queue_wait_ms = queued_submission_value.queue_wait_ms;
        return submission_stage_metrics_value;
    }

    submission_stage_metrics make_submission_stage_metrics(
        submission_stage_metrics submission_stage_metrics_value,
        const process_submission_data& process_submission_data_value
    ){
        submission_stage_metrics_value.testcase_count =
            process_submission_data_value.execution_report_value.testcase_count;
        submission_stage_metrics_value.compile_prepare_elapsed_ms =
            process_submission_data_value.execution_report_value.prepare_elapsed_ms;
        submission_stage_metrics_value.testcase_execution_elapsed_ms =
            process_submission_data_value.execution_report_value.testcase_execution_elapsed_ms;
        submission_stage_metrics_value.final_submission_status =
            judge_policy::to_submission_status(
                process_submission_data_value.judge_result_value
            );
        return submission_stage_metrics_value;
    }

    submission_stage_metrics make_requeued_submission_stage_metrics(
        const submission_dto::queued_submission& queued_submission_value,
        std::string error_message
    ){
        submission_stage_metrics submission_stage_metrics_value = make_submission_stage_metrics(
            queued_submission_value
        );
        submission_stage_metrics_value.event = "requeued";
        submission_stage_metrics_value.error_message_opt = std::move(error_message);
        return submission_stage_metrics_value;
    }

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
