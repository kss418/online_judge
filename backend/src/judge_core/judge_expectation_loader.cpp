#include "judge_core/judge_expectation_loader.hpp"

#include "common/file_util.hpp"

#include <utility>

std::expected<judge_expectation, io_error> judge_expectation_loader::load(
    const testcase_snapshot& testcase_snapshot_value
){
    const auto validated_testcase_count_exp = testcase_snapshot_value.validate_testcase_layout();
    if(!validated_testcase_count_exp){
        return std::unexpected(validated_testcase_count_exp.error());
    }

    judge_expectation judge_expectation_value;
    judge_expectation_value.expected_output_texts.reserve(
        static_cast<std::size_t>(*validated_testcase_count_exp)
    );

    for(std::int32_t order = 1; order <= *validated_testcase_count_exp; ++order){
        const auto answer_path_exp = testcase_snapshot_value.make_output_path(order);
        if(!answer_path_exp){
            return std::unexpected(answer_path_exp.error());
        }

        const auto answer_text_exp = file_util::read_file_content(*answer_path_exp);
        if(!answer_text_exp){
            return std::unexpected(answer_text_exp.error());
        }

        judge_expectation_value.expected_output_texts.push_back(
            std::move(*answer_text_exp)
        );
    }

    return judge_expectation_value;
}
