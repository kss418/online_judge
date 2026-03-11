#include "judge_server/testcase_runner.hpp"

#include "common/file_util.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <chrono>
#include <utility>
#include <vector>

namespace tc_runner{
    constexpr std::chrono::milliseconds source_run_time_limit{2000};
    constexpr std::int64_t source_run_memory_limit_mb = 256;

    std::expected<std::filesystem::path, error_code> make_tc_input_path(
        std::int64_t problem_id,
        std::int32_t order
    );

    std::expected<sandbox_runner::run_result, error_code> run_one_tc(
        const pl_runner_util::prepared_source& prepared_source_value,
        const std::filesystem::path& input_path
    );
}

std::expected<std::filesystem::path, error_code> tc_runner::make_tc_input_path(
    std::int64_t problem_id,
    std::int32_t order
){
    return file_util::instance().make_tc_input_path(problem_id, order);
}

std::expected<sandbox_runner::run_result, error_code> tc_runner::run_one_tc(
    const pl_runner_util::prepared_source& prepared_source_value,
    const std::filesystem::path& input_path
){
    if(!prepared_source_value.is_runnable()){
        return *prepared_source_value.compile_failed_run_result_;
    }

    return sandbox_runner::run(
        prepared_source_value.run_command_args_,
        input_path,
        source_run_time_limit,
        source_run_memory_limit_mb
    );
}

std::expected<std::vector<sandbox_runner::run_result>, error_code> tc_runner::run_all_tcs(
    const std::filesystem::path& source_file_path,
    std::int64_t problem_id
){
    const auto tc_count_exp = file_util::instance().count_tc_output(problem_id);
    if(!tc_count_exp){
        return std::unexpected(tc_count_exp.error());
    }

    const auto validated_tc_count_exp = file_util::instance().validate_tc_output(
        problem_id,
        tc_count_exp.value()
    );
    if(!validated_tc_count_exp){
        return std::unexpected(validated_tc_count_exp.error());
    }

    const auto prepare_source_exp = pl_runner_util::instance().prepare_source(source_file_path);
    if(!prepare_source_exp){
        return std::unexpected(prepare_source_exp.error());
    }

    if(!prepare_source_exp->is_runnable()){
        std::vector<sandbox_runner::run_result> run_results;
        run_results.push_back(*prepare_source_exp->compile_failed_run_result_);
        return run_results;
    }

    std::vector<sandbox_runner::run_result> run_results;
    run_results.reserve(static_cast<std::size_t>(*validated_tc_count_exp));

    for(std::int32_t order = 1; order <= *validated_tc_count_exp; ++order){
        const auto input_path_exp = make_tc_input_path(problem_id, order);
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto run_one_tc_exp = run_one_tc(*prepare_source_exp, *input_path_exp);
        if(!run_one_tc_exp){
            return std::unexpected(run_one_tc_exp.error());
        }

        run_results.push_back(std::move(*run_one_tc_exp));
    }

    return run_results;
}
