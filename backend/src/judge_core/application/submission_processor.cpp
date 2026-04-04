#include "judge_core/application/submission_processor.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"
#include "judge_core/policy/judge_policy.hpp"

#include <utility>

std::expected<submission_processor, judge_error> submission_processor::create(
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

    return submission_processor(
        std::move(dependencies_value.testcase_snapshot_connection),
        std::move(dependencies_value.submission_execution_service_value),
        std::move(dependencies_value.source_root_path)
    );
}

submission_processor::submission_processor(
    db_connection testcase_snapshot_connection,
    submission_execution_service submission_execution_service,
    std::filesystem::path source_root_path
) :
    testcase_snapshot_connection_(std::move(testcase_snapshot_connection)),
    submission_execution_service_(std::move(submission_execution_service)),
    source_root_path_(std::move(source_root_path)){}

submission_processor::submission_processor(
    submission_processor&& other
) noexcept = default;

submission_processor& submission_processor::operator=(
    submission_processor&& other
) noexcept = default;

submission_processor::~submission_processor() = default;

std::expected<void, judge_error> submission_processor::process_submission(
    judge_submission_port& judge_submission_port_value,
    const submission_dto::queued_submission& queued_submission_value
){
    const auto workspace_path_exp = judge_workspace::make_submission_workspace_path(
        source_root_path_,
        queued_submission_value.submission_id
    );
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    const auto execute_submission_exp = execute_submission(
        judge_submission_port_value,
        queued_submission_value,
        *workspace_path_exp
    );
    const auto cleanup_workspace_exp = judge_workspace::cleanup(*workspace_path_exp);
    if(!cleanup_workspace_exp){
        return std::unexpected(judge_error{cleanup_workspace_exp.error()});
    }

    if(!execute_submission_exp){
        const auto requeue_submission_exp = requeue_submission(
            judge_submission_port_value,
            queued_submission_value.submission_id,
            to_string(execute_submission_exp.error())
        );
        if(!requeue_submission_exp){
            return std::unexpected(requeue_submission_exp.error());
        }
    }

    return {};
}

std::expected<void, judge_error> submission_processor::execute_submission(
    judge_submission_port& judge_submission_port_value,
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    const auto mark_judging_exp =
        judge_submission_port_value.mark_judging(
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
        judge_submission_port_value,
        queued_submission_value.submission_id,
        judge_submission_exp->judge_result_value,
        judge_submission_exp->execution_report_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<void, judge_error> submission_processor::finalize_submission(
    judge_submission_port& judge_submission_port_value,
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

    const auto finalize_submission_exp =
        judge_submission_port_value.finalize_submission(finalize_request_value);
    if(!finalize_submission_exp){
        return std::unexpected(finalize_submission_exp.error());
    }

    return {};
}

std::expected<void, judge_error> submission_processor::requeue_submission(
    judge_submission_port& judge_submission_port_value,
    std::int64_t submission_id,
    std::string reason
){
    return judge_submission_port_value.requeue_submission_immediately(
        submission_id,
        std::move(reason)
    );
}
