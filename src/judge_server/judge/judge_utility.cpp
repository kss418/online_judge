#include "judge_server/judge/judge_utility.hpp"

bool judge_utility::is_blank(char c){
    return (c == ' ' || c == '\t' || c == '\r');
}

std::vector <std::string> judge_utility::normalize_output(const std::string& output){
    std::vector <std::string> ret;
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
