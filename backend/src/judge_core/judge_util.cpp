#include "judge_core/judge_util.hpp"

#include "common/language_util.hpp"

#include <cstdlib>

namespace{
    io_error make_invalid_argument_error(const char* message){
        return io_error{io_error_code::invalid_argument, message};
    }
}

judge_util& judge_util::instance(){
    static judge_util judge_util_value;
    judge_util_value.initialize_if_needed();
    return judge_util_value;
}

void judge_util::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(source_directory_path_.has_value()){
        return;
    }

    const char* source_directory_path_env = std::getenv("JUDGE_SOURCE_ROOT");
    if(source_directory_path_env != nullptr && *source_directory_path_env != '\0'){
        source_directory_path_ = std::filesystem::path(source_directory_path_env);
    }
}

std::expected<std::filesystem::path, io_error> judge_util::make_source_directory_path(){
    initialize_if_needed();
    if(!source_directory_path_.has_value() || source_directory_path_->empty()){
        return std::unexpected(
            make_invalid_argument_error("judge source root is not configured")
        );
    }

    return *source_directory_path_;
}

std::expected<std::filesystem::path, io_error> judge_util::make_submission_workspace_path(
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(make_invalid_argument_error("invalid submission id"));
    }

    const auto source_root_path_exp = make_source_directory_path();
    if(!source_root_path_exp){
        return std::unexpected(source_root_path_exp.error());
    }

    return *source_root_path_exp / std::to_string(submission_id);
}

std::expected<std::filesystem::path, io_error> judge_util::make_source_file_path(
    std::int64_t submission_id,
    std::string_view language
){
    const auto workspace_path_exp = make_submission_workspace_path(submission_id);
    if(!workspace_path_exp){
        return std::unexpected(workspace_path_exp.error());
    }

    const auto supported_language_opt = language_util::find_supported_language(language);
    if(!supported_language_opt){
        return std::unexpected(make_invalid_argument_error("unsupported language"));
    }

    if(supported_language_opt->language == "java"){
        return *workspace_path_exp / "Main.java";
    }

    return *workspace_path_exp / ("main" + std::string(supported_language_opt->source_extension));
}

std::filesystem::path judge_util::sandbox_workspace_path() const{
    return "/workspace";
}

std::filesystem::path judge_util::make_sandbox_path(
    const std::filesystem::path& host_workspace_path,
    const std::filesystem::path& host_path
) const{
    const std::filesystem::path relative_path = host_path.lexically_relative(host_workspace_path);
    if(
        relative_path.empty() ||
        relative_path.native().starts_with("..")
    ){
        return {};
    }

    return sandbox_workspace_path() / relative_path;
}

bool judge_util::is_blank(char c){
    return (c == ' ' || c == '\t' || c == '\r');
}

std::vector<std::string> judge_util::normalize_output(const std::string& output){
    std::vector<std::string> ret;
    std::string tmp;
    for(char i : output){
        if(i == '\n'){
            while(!tmp.empty() && is_blank(tmp.back())) tmp.pop_back();
            ret.push_back(tmp);
            tmp.clear();
        }
        else{
            tmp.push_back(i);
        }
    }

    if(!output.empty() && output.back() != '\n'){
        while(!tmp.empty() && is_blank(tmp.back())) tmp.pop_back();
        ret.push_back(tmp);
    }

    while(!ret.empty() && ret.back().empty()){
        ret.pop_back();
    }

    return ret;
}
