#include "judge_core/application/judge_worker.hpp"

#include "common/logger.hpp"
#include "common/timer.hpp"
#include "judge_core/application/judge_service.hpp"
#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/policy/judge_policy.hpp"
#include "judge_core/types/judge_submission_data.hpp"

#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace{
    void log_submission_stage_metrics(
        std::int64_t submission_id,
        const judge_submission_data::submission_stage_metrics& submission_stage_metrics_value
    ){
        logger::clog()
            .log("submission_stage_metrics")
            .field("thread_id", std::this_thread::get_id())
            .field("submission_id", submission_id)
            .field("event", submission_stage_metrics_value.event)
            .field(
                "final_status",
                to_string(submission_stage_metrics_value.final_submission_status)
            )
            .field("queue_wait_ms", submission_stage_metrics_value.queue_wait_ms)
            .field(
                "prepare_workspace_ms",
                submission_stage_metrics_value.prepare_workspace_elapsed_ms
            )
            .field(
                "testcase_snapshot_ms",
                submission_stage_metrics_value.testcase_snapshot_elapsed_ms
            )
            .field(
                "compile_prepare_ms",
                submission_stage_metrics_value.compile_prepare_elapsed_ms
            )
            .field(
                "testcase_run_ms",
                submission_stage_metrics_value.testcase_execution_elapsed_ms
            )
            .field("finalize_ms", submission_stage_metrics_value.finalize_elapsed_ms)
            .field("cleanup_ms", submission_stage_metrics_value.cleanup_elapsed_ms)
            .field("total_ms", submission_stage_metrics_value.total_elapsed_ms)
            .field("testcase_count", submission_stage_metrics_value.testcase_count)
            .optional_field("error", submission_stage_metrics_value.error_message_opt);
    }
} // namespace

std::expected<judge_worker, judge_error> judge_worker::create(
    dependencies dependencies_value
){
    if(dependencies_value.source_root_path.empty()){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "judge source root is not configured"
            }
        );
    }

    return judge_worker(
        std::move(dependencies_value.submission_queue_source_value),
        std::move(dependencies_value.submission_db_connection),
        std::move(dependencies_value.testcase_snapshot_connection),
        std::move(dependencies_value.submission_execution_service_value),
        std::move(dependencies_value.source_root_path)
    );
}

judge_worker::judge_worker(
    submission_queue_source submission_queue_source,
    db_connection submission_db_connection,
    db_connection testcase_snapshot_connection,
    submission_execution_service submission_execution_service,
    std::filesystem::path source_root_path
) :
    submission_queue_source_(std::move(submission_queue_source)),
    db_connection_(std::move(submission_db_connection)),
    testcase_snapshot_connection_(std::move(testcase_snapshot_connection)),
    submission_execution_service_(std::move(submission_execution_service)),
    source_root_path_(std::move(source_root_path)){}

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
        const auto workspace_path_exp = judge_workspace::make_submission_workspace_path(
            source_root_path_,
            queued_submission_value.submission_id
        );
        if(!workspace_path_exp){
            return std::unexpected(workspace_path_exp.error());
        }

        timer processing_timer;
        const auto process_submission_exp = process_submission(
            queued_submission_value,
            *workspace_path_exp
        );

        judge_submission_data::submission_stage_metrics submission_stage_metrics_value;
        if(process_submission_exp){
            submission_stage_metrics_value = *process_submission_exp;
        }
        else{
            submission_stage_metrics_value =
                judge_submission_data::make_requeued_submission_stage_metrics(
                    queued_submission_value,
                    to_string(process_submission_exp.error())
                );
        }

        const auto cleanup_workspace_exp = timer::measure_elapsed_ms(
            submission_stage_metrics_value.cleanup_elapsed_ms,
            [&workspace_path_exp]() -> std::expected<void, judge_error> {
                const auto cleanup_workspace_exp = judge_workspace::cleanup(
                    *workspace_path_exp
                );
                if(!cleanup_workspace_exp){
                    return std::unexpected(judge_error{cleanup_workspace_exp.error()});
                }

                return {};
            }
        );
        if(!cleanup_workspace_exp){
            return std::unexpected(cleanup_workspace_exp.error());
        }

        if(!process_submission_exp){
            const auto requeue_submission_exp = requeue_submission(
                queued_submission_value.submission_id,
                *submission_stage_metrics_value.error_message_opt
            );
            if(!requeue_submission_exp){
                return std::unexpected(requeue_submission_exp.error());
            }
        }

        submission_stage_metrics_value.total_elapsed_ms = processing_timer.elapsed_ms();
        log_submission_stage_metrics(
            queued_submission_value.submission_id,
            submission_stage_metrics_value
        );
    }
}

std::expected<judge_submission_data::submission_stage_metrics, judge_error>
judge_worker::process_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    auto submission_stage_metrics_value =
        judge_submission_data::make_submission_stage_metrics(
            queued_submission_value
        );

    const auto mark_judging_exp = judge_service::mark_judging(
        db_connection_,
        queued_submission_value.submission_id
    );
    if(!mark_judging_exp){
        return std::unexpected(mark_judging_exp.error());
    }

    auto judge_submission_exp = submission_execution_service_.process_submission(
        queued_submission_value,
        workspace_path,
        testcase_snapshot_connection_
    );
    if(!judge_submission_exp){
        return std::unexpected(judge_submission_exp.error());
    }
    submission_stage_metrics_value = judge_submission_data::make_submission_stage_metrics(
        std::move(submission_stage_metrics_value),
        *judge_submission_exp
    );

    const auto finalize_submission_exp = timer::measure_elapsed_ms(
        submission_stage_metrics_value.finalize_elapsed_ms,
        [this, &queued_submission_value, &judge_submission_exp]{
            return finalize_submission(
                queued_submission_value.submission_id,
                judge_submission_exp->judge_result_value,
                judge_submission_exp->execution_report_value
            );
        }
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return submission_stage_metrics_value;
}

std::expected<void, judge_error> judge_worker::finalize_submission(
    std::int64_t submission_id,
    judge_result result,
    const execution_report::batch& execution_report_value
){
    const submission_status submission_status_value =
        judge_policy::to_submission_status(result);
    const auto finalize_submission_data_value =
        judge_policy::make_finalize_submission_data(
            submission_status_value,
            execution_report_value
        );
    const submission_dto::finalize_request finalize_request_value =
        submission_dto::make_finalize_request(
            submission_id,
            submission_status_value,
            finalize_submission_data_value.score,
            finalize_submission_data_value.compile_output,
            finalize_submission_data_value.judge_output,
            finalize_submission_data_value.elapsed_ms_opt,
            finalize_submission_data_value.max_rss_kb_opt
        );

    const auto finalize_submission_exp = judge_service::finalize_submission(
        db_connection_,
        finalize_request_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<void, judge_error> judge_worker::requeue_submission(
    std::int64_t submission_id,
    std::string reason
){
    return judge_service::requeue_submission_immediately(
        db_connection_,
        submission_id,
        std::move(reason)
    );
}
