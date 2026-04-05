#include "judge_core/application/workspace_runner.hpp"

#include "common/file_util.hpp"
#include "common/language_util.hpp"

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

    std::expected<std::filesystem::path, judge_error> make_source_file_path(
        const std::filesystem::path& workspace_path,
        std::string_view language
    ){
        const auto workspace_path_validation_exp = require_non_empty_path(
            workspace_path,
            "workspace path is empty"
        );
        if(!workspace_path_validation_exp){
            return std::unexpected(workspace_path_validation_exp.error());
        }

        const auto supported_language_opt =
            language_util::find_supported_language(language);
        if(!supported_language_opt){
            return std::unexpected(make_validation_error("unsupported language"));
        }

        if(supported_language_opt->language == "java"){
            return workspace_path / "Main.java";
        }

        return workspace_path /
               ("main" + std::string(supported_language_opt->source_extension));
    }
}

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

std::expected<std::filesystem::path, judge_error> workspace_runner::make_workspace_path(
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(make_validation_error("invalid submission id"));
    }

    return source_root_path_ / std::to_string(submission_id);
}

std::expected<std::filesystem::path, judge_error> workspace_runner::write_source_file(
    std::int64_t submission_id,
    std::string_view language,
    std::string_view source_code
){
    const auto workspace_path_exp = make_workspace_path(submission_id);
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    const auto source_file_path_exp = make_source_file_path(
        *workspace_path_exp,
        language
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    const auto create_file_exp = file_util::create_file(
        *source_file_path_exp,
        source_code
    );
    if(!create_file_exp){
        return std::unexpected(judge_error{create_file_exp.error()});
    }

    return *source_file_path_exp;
}

std::expected<std::filesystem::path, judge_error> workspace_runner::prepare_workspace(
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

std::expected<void, judge_error> workspace_runner::cleanup_workspace(
    const std::filesystem::path& workspace_path
){
    const auto workspace_path_validation_exp = require_non_empty_path(
        workspace_path,
        "workspace path is empty"
    );
    if(!workspace_path_validation_exp){
        return std::unexpected(workspace_path_validation_exp.error());
    }

    const auto cleanup_workspace_exp = file_util::remove_all(workspace_path);
    if(!cleanup_workspace_exp){
        return std::unexpected(judge_error{cleanup_workspace_exp.error()});
    }

    return {};
}
