#include "judge_core/infrastructure/workspace_path_mapper.hpp"

std::filesystem::path workspace_path_mapper::sandbox_workspace_path(){
    return "/workspace";
}

std::filesystem::path workspace_path_mapper::make_sandbox_path(
    const std::filesystem::path& host_workspace_path,
    const std::filesystem::path& host_path
){
    const std::filesystem::path relative_path =
        host_path.lexically_relative(host_workspace_path);
    if(
        relative_path.empty() ||
        relative_path.native().starts_with("..")
    ){
        return {};
    }

    return workspace_path_mapper::sandbox_workspace_path() / relative_path;
}
