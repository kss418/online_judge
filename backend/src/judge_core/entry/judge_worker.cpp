#include "judge_core/entry/judge_worker.hpp"

#include <utility>

std::expected<judge_worker, judge_error> judge_worker::create(
    dependencies dependencies_value
){
    return judge_worker(std::move(dependencies_value.submission_processor_value));
}

judge_worker::judge_worker(submission_processor submission_processor_value) :
    submission_processor_(std::move(submission_processor_value)){}

std::expected<void, judge_error> judge_worker::run(){
    while(true){
        const auto process_submission_exp =
            submission_processor_.process_next_submission(
                LEASE_DURATION,
                NOTIFICATION_WAIT_TIMEOUT
            );
        if(!process_submission_exp){
            return std::unexpected(process_submission_exp.error());
        }
    }
}
