#include "judge_core/application/submission_execution_service.hpp"

#include "judge_core/policy/judge_expectation_loader.hpp"
#include "judge_core/policy/judge_policy.hpp"

#include <utility>

std::expected<submission_execution_service, judge_error>
submission_execution_service::create(
    testcase_snapshot_port testcase_snapshot_port_value
){
    auto execution_engine_exp = execution_engine::create(
        std::move(testcase_snapshot_port_value)
    );
    if(!execution_engine_exp){
        return std::unexpected(execution_engine_exp.error());
    }

    return submission_execution_service(std::move(*execution_engine_exp));
}

submission_execution_service::submission_execution_service(
    execution_engine execution_engine_value
) :
    execution_engine_(std::move(execution_engine_value)){}

submission_execution_service::submission_execution_service(
    submission_execution_service&& other
) noexcept = default;

submission_execution_service& submission_execution_service::operator=(
    submission_execution_service&& other
) noexcept = default;

submission_execution_service::~submission_execution_service() = default;

std::expected<judge_submission_data::process_submission_data, judge_error>
submission_execution_service::process_submission(
    const submission_dto::queued_submission& queued_submission_value,
    const std::filesystem::path& workspace_path
){
    auto execute_result_exp = execution_engine_.execute(
        queued_submission_value,
        workspace_path
    );
    if(!execute_result_exp){
        return std::unexpected(execute_result_exp.error());
    }

    auto execute_result_value = std::move(*execute_result_exp);
    if(auto* process_submission_data_value =
           std::get_if<judge_submission_data::process_submission_data>(
               &execute_result_value
           )){
        return std::move(*process_submission_data_value);
    }

    auto execution_result_value =
        std::get<execution_engine::execution_result>(std::move(execute_result_value));
    const auto judge_expectation_exp = judge_expectation_loader::load(
        execution_result_value.testcase_snapshot_value
    );
    if(!judge_expectation_exp){
        return std::unexpected(judge_error{judge_expectation_exp.error()});
    }

    const auto judge_result_exp = judge_policy::check_result(
        *judge_expectation_exp,
        execution_result_value.execution_report_value
    );
    if(!judge_result_exp){
        return std::unexpected(judge_result_exp.error());
    }

    auto process_submission_data_value =
        judge_submission_data::make_process_submission_data(
            *judge_result_exp,
            std::move(execution_result_value.execution_report_value)
        );
    return process_submission_data_value;
}
