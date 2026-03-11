#include "judge_server/judge_util.hpp"

#include <cstdlib>

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

std::expected<std::filesystem::path, error_code> judge_util::make_source_directory_path(){
    initialize_if_needed();
    if(!source_directory_path_.has_value() || source_directory_path_->empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return *source_directory_path_;
}

std::expected<std::filesystem::path, error_code> judge_util::make_source_file_path(
    std::int64_t submission_id,
    std::string_view language
){
    const auto source_root_path_exp = make_source_directory_path();
    if(!source_root_path_exp){
        return std::unexpected(source_root_path_exp.error());
    }

    std::string_view extension = ".txt";
    if(language == "cpp"){
        extension = ".cpp";
    }
    else if(language == "python"){
        extension = ".py";
    }
    else if(language == "java"){
        extension = ".java";
    }

    return *source_root_path_exp / (std::to_string(submission_id) + std::string(extension));
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
