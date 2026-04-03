#include "judge_core/policy/checker.hpp"

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

bool checker::check(
    std::string_view output,
    std::string_view expected_output
){
    return normalize_output(output) == normalize_output(expected_output);
}

judge_result checker::check_all(
    const std::vector<std::string>& output,
    const judge_expectation& judge_expectation_value
){
    if(output.size() != judge_expectation_value.expected_output_texts.size()){
        return judge_result::wrong_answer;
    }

    for(std::size_t index = 0; index < output.size(); ++index){
        if(
            !check(
                output[index],
                judge_expectation_value.expected_output_texts[index]
            )
        ){
            return judge_result::wrong_answer;
        }
    }

    return judge_result::accepted;
}
