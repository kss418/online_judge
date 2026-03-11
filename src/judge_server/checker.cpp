#include "judge_server/checker.hpp"

#include "common/unique_fd.hpp"
#include "common/blocking_io.hpp"
#include "common/file_util.hpp"
#include "judge_server/judge_util.hpp"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

std::expected<bool, error_code> checker::check(
    const std::vector<std::string>& output,
    const path& answer_path
){
    unique_fd answer_fd = unique_fd(open(answer_path.c_str(), O_RDONLY));
    if(!answer_fd){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    auto answer_text_exp = blocking_io::read_all(answer_fd.get());
    if(!answer_text_exp){
        return std::unexpected(answer_text_exp.error());
    }

    auto answer_text = std::move(*answer_text_exp);
    return output == judge_util::normalize_output(answer_text);
}

std::expected<judge_result, error_code> checker::check_all(
    const std::vector<std::vector<std::string>>& output, std::int64_t problem_id
){
    const auto testcase_count_exp = file_util::instance().count_testcase_output(problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    if(output.size() != static_cast<std::size_t>(testcase_count_exp.value())){
        return judge_result::wrong_answer;
    }

    const auto validated_testcase_count_exp = file_util::instance().validate_testcase_output(
        problem_id,
        testcase_count_exp.value()
    );
    
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= validated_testcase_count_exp.value(); ++order){
        const auto answer_path_exp = file_util::instance().make_testcase_output_path(
            problem_id, order
        );

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
