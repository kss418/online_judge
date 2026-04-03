#include "judge_core/application/judge_worker.hpp"

#include "judge_core/application/judge_service.hpp"
#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/policy/judge_policy.hpp"

#include <utility>

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

        const auto process_submission_exp = process_submission(
            queued_submission_value,
            *workspace_path_exp
        );
        const auto cleanup_workspace_exp = judge_workspace::cleanup(*workspace_path_exp);
        if(!cleanup_workspace_exp){
            return std::unexpected(judge_error{cleanup_workspace_exp.error()});
        }

        if(!process_submission_exp){
            const auto requeue_submission_exp = requeue_submission(
                queued_submission_value.submission_id,
                to_string(process_submission_exp.error())
            );
            if(!requeue_submission_exp){
                return std::unexpected(requeue_submission_exp.error());
            }
        }
    }
}

std::expected<void, judge_error>
judge_worker::process_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
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
    const auto finalize_submission_exp = finalize_submission(
        queued_submission_value.submission_id,
        judge_submission_exp->judge_result_value,
        judge_submission_exp->execution_report_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
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
