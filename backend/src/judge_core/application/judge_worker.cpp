#include "judge_core/application/judge_worker.hpp"

#include <utility>

std::expected<judge_worker, judge_error> judge_worker::create(
    dependencies dependencies_value
){
    auto submission_processor_exp = submission_processor::create(
        submission_processor::dependencies{
            .testcase_snapshot_connection = std::move(
                dependencies_value.testcase_snapshot_connection
            ),
            .submission_execution_service_value = std::move(
                dependencies_value.submission_execution_service_value
            ),
            .source_root_path = std::move(dependencies_value.source_root_path),
        }
    );
    if(!submission_processor_exp){
        return std::unexpected(submission_processor_exp.error());
    }

    return judge_worker(
        std::move(dependencies_value.judge_submission_port_value),
        std::move(dependencies_value.submission_queue_source_value),
        std::move(*submission_processor_exp)
    );
}

judge_worker::judge_worker(
    judge_submission_port judge_submission_port_value,
    submission_queue_source submission_queue_source,
    submission_processor submission_processor_value
) :
    judge_submission_port_(std::move(judge_submission_port_value)),
    submission_queue_source_(std::move(submission_queue_source)),
    submission_processor_(std::move(submission_processor_value)){}

std::expected<void, judge_error> judge_worker::run(){
    while(true){
        auto queued_submission_opt_exp =
            judge_submission_port_.lease_submission(LEASE_DURATION);
        if(!queued_submission_opt_exp){
            return std::unexpected(queued_submission_opt_exp.error());
        }

        if(queued_submission_opt_exp->has_value()){
            const submission_dto::queued_submission& queued_submission_value =
                queued_submission_opt_exp->value();
            const auto process_submission_exp =
                submission_processor_.process_submission(
                    judge_submission_port_,
                    queued_submission_value
                );
            if(!process_submission_exp){
                return std::unexpected(process_submission_exp.error());
            }
            continue;
        }

        const auto wait_submission_notification_exp =
            submission_queue_source_.wait_submission_notification(
                NOTIFICATION_WAIT_TIMEOUT
            );
        if(!wait_submission_notification_exp){
            return std::unexpected(
                judge_error{wait_submission_notification_exp.error()}
            );
        }
    }
}
