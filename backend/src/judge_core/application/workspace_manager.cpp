#include "judge_core/application/workspace_manager.hpp"

#include "common/file_util.hpp"

#include <utility>

namespace{
    judge_error make_validation_error(const char* message){
        return judge_error{judge_error_code::validation_error, message};
    }

    std::expected<void, judge_error> require_non_empty_path(
        const std::filesystem::path& path,
        const char* message
    ){
        if(path.empty()){
            return std::unexpected(make_validation_error(message));
        }

        return {};
    }

}

std::expected<workspace_manager, judge_error> workspace_manager::create(
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

    return workspace_manager(std::move(source_root_path));
}

workspace_manager::workspace_manager(
    std::filesystem::path source_root_path
) :
    source_root_path_(std::move(source_root_path)){}

workspace_manager::workspace_manager(
    workspace_manager&& other
) noexcept = default;

workspace_manager& workspace_manager::operator=(
    workspace_manager&& other
) noexcept = default;

workspace_manager::~workspace_manager() = default;

std::expected<workspace_session, judge_error> workspace_manager::create(
    std::int64_t submission_id
){
    const auto workspace_path_exp = prepare_workspace(submission_id);
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    return workspace_session(std::move(*workspace_path_exp));
}

std::expected<std::filesystem::path, judge_error> workspace_manager::make_workspace_path(
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(make_validation_error("invalid submission id"));
    }

    return source_root_path_ / std::to_string(submission_id);
}

std::expected<std::filesystem::path, judge_error> workspace_manager::prepare_workspace(
    std::int64_t submission_id
){
    const auto workspace_path_exp = make_workspace_path(submission_id);
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    const auto workspace_path_validation_exp = require_non_empty_path(
        *workspace_path_exp,
        "workspace path is empty"
    );
    if(!workspace_path_validation_exp){
        return std::unexpected(workspace_path_validation_exp.error());
    }

    const auto cleanup_workspace_exp = file_util::remove_all(*workspace_path_exp);
    if(!cleanup_workspace_exp){
        return std::unexpected(judge_error{cleanup_workspace_exp.error()});
    }

    const auto create_directories_exp = file_util::create_directories(*workspace_path_exp);
    if(!create_directories_exp){
        return std::unexpected(judge_error{create_directories_exp.error()});
    }

    return *workspace_path_exp;
}
