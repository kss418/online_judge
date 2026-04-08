#include "judge_core/application/expectation_reader.hpp"

#include "common/file_util.hpp"

#include <utility>

std::expected<judge_expectation, judge_error> expectation_reader::read(
    const testcase_snapshot& testcase_snapshot_value
) const{
    const auto validate_exp = testcase_snapshot_value.validate();
    if(!validate_exp){
        return std::unexpected(judge_error{validate_exp.error()});
    }

    judge_expectation judge_expectation_value;
    judge_expectation_value.expected_output_texts.reserve(
        static_cast<std::size_t>(testcase_snapshot_value.case_count())
    );

    for(std::int32_t order = 1; order <= testcase_snapshot_value.case_count(); ++order){
        const auto answer_path_exp = testcase_snapshot_value.output_path(order);
        if(!answer_path_exp){
            return std::unexpected(judge_error{answer_path_exp.error()});
        }

        const auto answer_text_exp = file_util::read_file_content(*answer_path_exp);
        if(!answer_text_exp){
            return std::unexpected(judge_error{answer_text_exp.error()});
        }

        judge_expectation_value.expected_output_texts.push_back(
            std::move(*answer_text_exp)
        );
    }

    return judge_expectation_value;
}
