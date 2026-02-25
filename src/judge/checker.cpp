#include "judge/checker.hpp"
#include "core/unique_fd.hpp"
#include "net/blocking_io.hpp"
#include "judge/judge_utility.hpp"

#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

std::expected <judge_result, error_code> checker::check(std::vector <std::string> output, const path& answer_path){
    unique_fd answer_fd = unique_fd(open(answer_path.c_str(), O_RDONLY));
    if(!answer_fd){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    auto answer_text_exp = blocking_io::read_all(answer_fd.get());
    if(!answer_text_exp){
        return std::unexpected(answer_text_exp.error());
    }

    auto answer_text = std::move(*answer_text_exp);
    return output == judge_utility::normalize_output(answer_text)
        ? judge_result::accepted
        : judge_result::wrong_answer;
};
