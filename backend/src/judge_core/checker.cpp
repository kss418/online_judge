#include "judge_core/checker.hpp"

#include "common/blocking_io.hpp"
#include "common/unique_fd.hpp"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace{
    bool is_blank(char c){
        return (c == ' ' || c == '\t' || c == '\r');
    }
}

std::vector<std::string> checker::normalize_output(std::string_view output){
    std::vector<std::string> ret;
    std::string tmp;
    for(char c : output){
        if(c == '\n'){
            while(!tmp.empty() && is_blank(tmp.back())) tmp.pop_back();
            ret.push_back(tmp);
            tmp.clear();
        }
        else{
            tmp.push_back(c);
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

std::expected<bool, io_error> checker::check(
    std::string_view output,
    const path& answer_path
){
    unique_fd answer_fd = unique_fd(open(answer_path.c_str(), O_RDONLY));
    if(!answer_fd){
        return std::unexpected(io_error::from_errno(errno));
    }

    auto answer_text_exp = blocking_io::read_all(answer_fd.get());
    if(!answer_text_exp){
        return std::unexpected(answer_text_exp.error());
    }

    auto answer_text = std::move(*answer_text_exp);
    return normalize_output(output) == normalize_output(answer_text);
}

std::expected<judge_result, io_error> checker::check_all(
    const std::vector<std::string>& output,
    const testcase_snapshot& testcase_snapshot_value
){
    if(output.size() != static_cast<std::size_t>(testcase_snapshot_value.testcase_count)){
        return judge_result::wrong_answer;
    }

    const auto validated_testcase_count_exp = testcase_snapshot_value.validate_testcase_layout();
    
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= validated_testcase_count_exp.value(); ++order){
        const auto answer_path_exp = testcase_snapshot_value.make_output_path(order);

        if(!answer_path_exp){
            return std::unexpected(answer_path_exp.error());
        }

        const auto check_exp = check(output[static_cast<std::size_t>(order - 1)], *answer_path_exp);
        if(!check_exp){
            return std::unexpected(check_exp.error());
        }

        if(!check_exp.value()){
            return judge_result::wrong_answer;
        }
    }

    return judge_result::accepted;
}
