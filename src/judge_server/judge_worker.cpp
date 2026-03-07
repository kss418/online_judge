#include "judge_server/judge_worker.hpp"
#include "common/file_utility.hpp"
#include "common/env_utility.hpp"
#include "common/temp_file.hpp"
#include "judge_server/code_runner.hpp"
#include "judge_server/compile_runner.hpp"

#include <string>
#include <utility>
#include <vector>

std::expected<judge_worker, error_code> judge_worker::create(submission_service submission_service){
    const auto require_envs_exp = env_utility::require_envs(
        {"JUDGE_CPP_COMPILER_PATH", "JUDGE_PYTHON_PATH", "JUDGE_JAVA_RUNTIME_PATH"}
    );
    if(!require_envs_exp){
        return std::unexpected(require_envs_exp.error());
    }

    auto cpp_compiler_path_exp = env_utility::require_env("JUDGE_CPP_COMPILER_PATH");
    if(!cpp_compiler_path_exp){
        return std::unexpected(cpp_compiler_path_exp.error());
    }
    auto python_path_exp = env_utility::require_env("JUDGE_PYTHON_PATH");
    if(!python_path_exp){
        return std::unexpected(python_path_exp.error());
    }
    auto java_runtime_path_exp = env_utility::require_env("JUDGE_JAVA_RUNTIME_PATH");
    if(!java_runtime_path_exp){
        return std::unexpected(java_runtime_path_exp.error());
    }

    const auto source_directory_path_exp = file_utility::instance().make_source_directory_path();
    if(!source_directory_path_exp){
        return std::unexpected(source_directory_path_exp.error());
    }

    const auto create_directories_exp = file_utility::instance().create_directories(
        *source_directory_path_exp
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    auto listen_submission_queue_exp = submission_service.listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    return judge_worker(
        std::move(submission_service),
        std::move(*cpp_compiler_path_exp),
        std::move(*python_path_exp),
        std::move(*java_runtime_path_exp)
    );
}

judge_worker::judge_worker(
    submission_service submission_service,
    std::string cpp_compiler_path,
    std::string python_path,
    std::string java_runtime_path
) :
    submission_service_(std::move(submission_service)),
    cpp_compiler_path_(std::move(cpp_compiler_path)),
    python_path_(std::move(python_path)),
    java_runtime_path_(std::move(java_runtime_path)){}

bool judge_worker::is_queue_empty_error(const error_code& code){
    return code.type_ == error_type::errno_type &&
        code.code_ == static_cast<int>(errno_error::resource_temporarily_unavailable);
}

std::expected<void, error_code> judge_worker::run(){
    while(true){
        auto save_source_code_exp = save_source_code();
        if(!save_source_code_exp){
            return std::unexpected(save_source_code_exp.error());
        }

        if(save_source_code_exp->has_value()){
            const queued_submission& queued_submission_value = save_source_code_exp->value();
            const auto set_testcase_paths_exp = set_testcase_paths(queued_submission_value.problem_id);
            if(!set_testcase_paths_exp){
                return std::unexpected(set_testcase_paths_exp.error());
            }
            const auto source_file_path_exp = file_utility::instance().make_source_file_path(
                queued_submission_value.submission_id,
                queued_submission_value.language
            );
            if(!source_file_path_exp){
                return std::unexpected(source_file_path_exp.error());
            }
            const std::filesystem::path source_file_path = *source_file_path_exp;
            auto run_source_code_exp = run_source_code(source_file_path);
            if(!run_source_code_exp){
                return std::unexpected(run_source_code_exp.error());
            }

            continue;
        }

        auto wait_submission_notification_exp = submission_service_.wait_submission_notification(
            notification_wait_timeout_
        );
        
        if(!wait_submission_notification_exp){
            return std::unexpected(wait_submission_notification_exp.error());
        }
    }
}

std::expected<std::optional<queued_submission>, error_code> judge_worker::save_source_code(){
    auto pop_submission_exp = submission_service_.pop_submission();
    if(!pop_submission_exp){
        if(is_queue_empty_error(pop_submission_exp.error())){
            return std::optional<queued_submission>{};
        }

        return std::unexpected(pop_submission_exp.error());
    }

    const auto source_file_path_exp = file_utility::instance().make_source_file_path(
        pop_submission_exp->submission_id,
        pop_submission_exp->language
    );
    if(!source_file_path_exp){
        return std::unexpected(source_file_path_exp.error());
    }
    const auto source_file_path = *source_file_path_exp;
    
    auto create_file_exp = file_utility::instance().create_file(
        source_file_path,
        pop_submission_exp->source_code
    );

    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return std::move(*pop_submission_exp);
}

std::expected<void, error_code> judge_worker::set_testcase_paths(std::int64_t problem_id){
    const auto input_path_exp = file_utility::instance().make_testcase_input_path(problem_id, 1);
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    input_path_ = std::move(*input_path_exp);

    const auto output_path_exp = file_utility::instance().make_testcase_output_path(problem_id, 1);
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    output_path_ = std::move(*output_path_exp);

    return {};
}

std::expected<code_runner::run_result, error_code> judge_worker::run_source_code(
    const std::filesystem::path& source_file_path
){
    const std::string extension = source_file_path.extension().string();
    if(extension == ".cpp"){
        return run_cpp(source_file_path);
    }
    if(extension == ".py"){
        return run_python(source_file_path);
    }
    if(extension == ".java"){
        return run_java(source_file_path);
    }

    return std::unexpected(error_code::create(errno_error::invalid_argument));
}

std::expected<code_runner::run_result, error_code> judge_worker::run_cpp(
    const std::filesystem::path& source_file_path
){
    auto binary_temp_exp = temp_file::create("/tmp/oj_binary_XXXXXX");
    if(!binary_temp_exp){
        return std::unexpected(binary_temp_exp.error());
    }

    auto compile_exp = compile_runner::compile_cpp(
        source_file_path,
        binary_temp_exp->get_path(),
        cpp_compiler_path_
    );

    if(!compile_exp){
        return std::unexpected(compile_exp.error());
    }

    if(compile_exp->exit_code_ != 0){
        code_runner::run_result run_result_value;
        run_result_value.exit_code_ = compile_exp->exit_code_;
        run_result_value.stderr_text_ = std::move(compile_exp->stderr_text_);
        return run_result_value;
    }

    binary_temp_exp->close_fd();
    std::vector<std::string> command_args = {binary_temp_exp->get_path().string()};
    auto run_exp = code_runner::run(
        command_args,
        input_path_,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );

    return run_exp;
}

std::expected<code_runner::run_result, error_code> judge_worker::run_python(
    const std::filesystem::path& source_file_path
){
    std::vector<std::string> command_args = {python_path_, source_file_path.string()};
    return code_runner::run(
        command_args,
        input_path_,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}

std::expected<code_runner::run_result, error_code> judge_worker::run_java(
    const std::filesystem::path& source_file_path
){
    std::vector<std::string> command_args = {java_runtime_path_, source_file_path.string()};
    return code_runner::run(
        command_args,
        input_path_,
        source_run_time_limit_,
        source_run_memory_limit_mb_
    );
}
