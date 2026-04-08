#include "judge_core/application/workspace_session.hpp"

#include "common/file_util.hpp"
#include "judge_core/types/workspace_layout.hpp"

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

    std::expected<void, judge_error> cleanup_workspace_path(
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

    void release_workspace_if_needed(
        std::filesystem::path& workspace_path,
        bool& closed
    ){
        if(!closed && !workspace_path.empty()){
            (void)cleanup_workspace_path(workspace_path);
        }

        workspace_path.clear();
        closed = true;
    }
}

workspace_session::workspace_session(
    std::filesystem::path workspace_path
) :
    workspace_path_(std::move(workspace_path)){}

workspace_session::workspace_session(
    workspace_session&& other
) noexcept :
    workspace_path_(std::move(other.workspace_path_)),
    closed_(other.closed_){
    other.workspace_path_.clear();
    other.closed_ = true;
}

workspace_session& workspace_session::operator=(
    workspace_session&& other
) noexcept{
    if(this == &other){
        return *this;
    }

    release_workspace_if_needed(workspace_path_, closed_);

    workspace_path_ = std::move(other.workspace_path_);
    closed_ = other.closed_;

    other.workspace_path_.clear();
    other.closed_ = true;
    return *this;
}

workspace_session::~workspace_session(){
    release_workspace_if_needed(workspace_path_, closed_);
}

const std::filesystem::path& workspace_session::path() const noexcept{
    return workspace_path_;
}

std::filesystem::path workspace_session::sandbox_root() const{
    return workspace_layout::sandbox_root_path();
}

std::expected<std::filesystem::path, judge_error> workspace_session::source_path_for(
    std::string_view language
) const{
    if(closed_){
        return std::unexpected(make_validation_error("workspace is already closed"));
    }

    const auto workspace_layout_exp = workspace_layout::create(workspace_path_);
    if(!workspace_layout_exp){
        return std::unexpected(judge_error{workspace_layout_exp.error()});
    }

    const auto source_path_exp = workspace_layout_exp->source_path_for(language);
    if(!source_path_exp){
        return std::unexpected(judge_error{source_path_exp.error()});
    }

    return *source_path_exp;
}

std::expected<std::filesystem::path, judge_error> workspace_session::sandbox_path_for(
    const std::filesystem::path& host_path
) const{
    if(closed_){
        return std::unexpected(make_validation_error("workspace is already closed"));
    }

    const auto workspace_layout_exp = workspace_layout::create(workspace_path_);
    if(!workspace_layout_exp){
        return std::unexpected(judge_error{workspace_layout_exp.error()});
    }

    const auto sandbox_path_exp = workspace_layout_exp->sandbox_path_for(host_path);
    if(!sandbox_path_exp){
        return std::unexpected(judge_error{sandbox_path_exp.error()});
    }

    return *sandbox_path_exp;
}

std::expected<std::filesystem::path, judge_error> workspace_session::write_source_file(
    std::string_view language,
    std::string_view source_code
){
    const auto source_file_path_exp = source_path_for(language);
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

std::expected<void, judge_error> workspace_session::close(){
    if(closed_){
        return {};
    }

    const auto cleanup_workspace_exp = cleanup_workspace_path(workspace_path_);
    if(!cleanup_workspace_exp){
        return std::unexpected(cleanup_workspace_exp.error());
    }

    workspace_path_.clear();
    closed_ = true;
    return {};
}
