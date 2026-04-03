#include "judge_core/infrastructure/judge_workspace.hpp"

#include "common/file_util.hpp"
#include "common/language_util.hpp"

namespace{
    io_error make_invalid_argument_error(const char* message){
        return io_error{io_error_code::invalid_argument, message};
    }

    std::expected<void, io_error> require_non_empty_path(
        const std::filesystem::path& path,
        const char* message
    ){
        if(path.empty()){
            return std::unexpected(make_invalid_argument_error(message));
        }

        return {};
    }
}

std::expected<std::filesystem::path, io_error> judge_workspace::make_submission_workspace_path(
    const std::filesystem::path& source_root_path,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(make_invalid_argument_error("invalid submission id"));
    }

    return source_root_path / std::to_string(submission_id);
}

std::expected<void, io_error> judge_workspace::reset(
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
        return std::unexpected(cleanup_workspace_exp.error());
    }

    return file_util::create_directories(workspace_path);
}

std::expected<std::filesystem::path, io_error> judge_workspace::make_source_file_path(
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

    const auto supported_language_opt = language_util::find_supported_language(language);
    if(!supported_language_opt){
        return std::unexpected(make_invalid_argument_error("unsupported language"));
    }

    if(supported_language_opt->language == "java"){
        return workspace_path / "Main.java";
    }

    return workspace_path / ("main" + std::string(supported_language_opt->source_extension));
}

std::expected<std::filesystem::path, io_error> judge_workspace::write_source_file(
    const std::filesystem::path& workspace_path,
    std::string_view language,
    std::string_view source_code
){
    const auto source_file_path_exp = make_source_file_path(workspace_path, language);
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }

    const auto create_file_exp = file_util::create_file(
        *source_file_path_exp,
        source_code
    );
    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return *source_file_path_exp;
}

std::expected<void, io_error> judge_workspace::cleanup(
    const std::filesystem::path& workspace_path
){
    const auto workspace_path_validation_exp = require_non_empty_path(
        workspace_path,
        "workspace path is empty"
    );
    if(!workspace_path_validation_exp){
        return std::unexpected(workspace_path_validation_exp.error());
    }

    return file_util::remove_all(workspace_path);
}

std::filesystem::path judge_workspace::sandbox_workspace_path(){
    return "/workspace";
}

std::filesystem::path judge_workspace::make_sandbox_path(
    const std::filesystem::path& host_workspace_path,
    const std::filesystem::path& host_path
){
    const std::filesystem::path relative_path = host_path.lexically_relative(host_workspace_path);
    if(
        relative_path.empty() ||
        relative_path.native().starts_with("..")
    ){
        return {};
    }

    return judge_workspace::sandbox_workspace_path() / relative_path;
}
