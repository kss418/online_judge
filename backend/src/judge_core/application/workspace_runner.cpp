#include "judge_core/application/workspace_runner.hpp"

#include "judge_core/infrastructure/judge_workspace.hpp"

#include <utility>

std::expected<workspace_runner, judge_error> workspace_runner::create(
    std::filesystem::path source_root_path
){
    if(source_root_path.empty()){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "judge source root is not configured"
            }
        );
    }

    return workspace_runner(std::move(source_root_path));
}

workspace_runner::workspace_runner(
    std::filesystem::path source_root_path
) :
    source_root_path_(std::move(source_root_path)){}

workspace_runner::workspace_runner(
    workspace_runner&& other
) noexcept = default;

workspace_runner& workspace_runner::operator=(
    workspace_runner&& other
) noexcept = default;

workspace_runner::~workspace_runner() = default;

std::expected<std::filesystem::path, judge_error> workspace_runner::prepare_workspace(
    std::int64_t submission_id
){
    const auto workspace_path_exp = judge_workspace::make_submission_workspace_path(
        source_root_path_,
        submission_id
    );
    if(!workspace_path_exp){
        return std::unexpected(judge_error{workspace_path_exp.error()});
    }

    const auto reset_workspace_exp = judge_workspace::reset(*workspace_path_exp);
    if(!reset_workspace_exp){
        return std::unexpected(judge_error{reset_workspace_exp.error()});
    }

    return *workspace_path_exp;
}

std::expected<void, judge_error> workspace_runner::cleanup_workspace(
    const std::filesystem::path& workspace_path
){
    const auto cleanup_workspace_exp = judge_workspace::cleanup(workspace_path);
    if(!cleanup_workspace_exp){
        return std::unexpected(judge_error{cleanup_workspace_exp.error()});
    }

    return {};
}
