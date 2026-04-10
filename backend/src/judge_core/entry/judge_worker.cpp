#include "judge_core/entry/judge_worker.hpp"

#include <utility>

std::expected<judge_worker, judge_error> judge_worker::create(
    dependencies dependencies_value
){
    if(!dependencies_value.judge_runtime_registry_value){
        return std::unexpected(judge_error::validation_error);
    }

    return judge_worker(
        std::move(dependencies_value.judge_queue_facade_value),
        std::move(dependencies_value.submission_processor_value),
        std::move(dependencies_value.judge_runtime_registry_value)
    );
}

judge_worker::judge_worker(
    judge_queue_facade judge_queue_facade_value,
    submission_processor submission_processor_value,
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry
) :
    judge_queue_facade_(std::move(judge_queue_facade_value)),
    submission_processor_(std::move(submission_processor_value)),
    judge_runtime_registry_(std::move(judge_runtime_registry)){}

std::expected<void, judge_error> judge_worker::run(){
    while(true){
        auto leased_submission_opt_exp = judge_queue_facade_.try_lease_next(
            LEASE_DURATION
        );
        if(!leased_submission_opt_exp){
            return std::unexpected(leased_submission_opt_exp.error());
        }
        if(!leased_submission_opt_exp->has_value()){
            const auto wait_for_work_exp = judge_queue_facade_.wait_for_work(
                NOTIFICATION_WAIT_TIMEOUT
            );
            if(!wait_for_work_exp){
                return std::unexpected(wait_for_work_exp.error());
            }
            continue;
        }

        auto busy_worker_guard = judge_runtime_registry_->mark_worker_busy();
        const auto process_submission_exp = submission_processor_.process(
            leased_submission_opt_exp->value()
        );
        if(!process_submission_exp){
            return std::unexpected(process_submission_exp.error());
        }
    }
}
