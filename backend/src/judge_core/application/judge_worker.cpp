#include "judge_core/application/judge_worker.hpp"

#include "judge_core/application/judge_service.hpp"

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
        std::move(dependencies_value.submission_queue_source_value),
        std::move(dependencies_value.submission_db_connection),
        std::move(*submission_processor_exp)
    );
}

judge_worker::judge_worker(
    submission_queue_source submission_queue_source,
    db_connection submission_db_connection,
    submission_processor submission_processor_value
) :
    submission_queue_source_(std::move(submission_queue_source)),
    db_connection_(std::move(submission_db_connection)),
    submission_processor_(std::move(submission_processor_value)){}

std::expected<void, judge_error> judge_worker::run(){
    while(true){
        auto queued_submission_opt_exp = judge_service::poll_next_submission(
            db_connection_,
            submission_queue_source_,
            LEASE_DURATION,
            NOTIFICATION_WAIT_TIMEOUT
        );
        if(!queued_submission_opt_exp){
            return std::unexpected(queued_submission_opt_exp.error());
        }

        if(!queued_submission_opt_exp->has_value()){
            continue;
        }

        const submission_dto::queued_submission& queued_submission_value =
            queued_submission_opt_exp->value();
        const auto process_submission_exp =
            submission_processor_.process_submission(
                db_connection_,
                queued_submission_value
            );
        if(!process_submission_exp){
            return std::unexpected(process_submission_exp.error());
        }
    }
}
