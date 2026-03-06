#include "judge_server/judge_worker.hpp"
#include "common/file_utility.hpp"
#include "common/env_utility.hpp"
#include "common/temp_file.hpp"
#include "judge_server/code_runner.hpp"
#include "judge_server/compile_runner.hpp"

#include <string>
#include <system_error>
#include <utility>
#include <vector>

std::expected<judge_worker, error_code> judge_worker::create(submission_service submission_service){
    auto source_root_exp = env_utility::require_env("JUDGE_SOURCE_ROOT");
    if(!source_root_exp){
        return std::unexpected(source_root_exp.error());
    }
    auto input_path_exp = env_utility::require_env("JUDGE_INPUT_PATH");
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    auto answer_path_exp = env_utility::require_env("JUDGE_ANSWER_PATH");
    if(!answer_path_exp){
        return std::unexpected(answer_path_exp.error());
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

    std::filesystem::path source_root_path = *source_root_exp;
    std::filesystem::path input_path = *input_path_exp;
    std::filesystem::path answer_path = *answer_path_exp;
    if(source_root_path.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    if(input_path.empty() || answer_path.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::error_code create_directory_ec;
    std::filesystem::create_directories(source_root_path, create_directory_ec);
    if(create_directory_ec){
        return std::unexpected(error_code::create(error_code::map_errno(create_directory_ec.value())));
    }

    auto listen_submission_queue_exp = submission_service.listen_submission_queue();
    if(!listen_submission_queue_exp){
        return std::unexpected(listen_submission_queue_exp.error());
    }

    return judge_worker(
        std::move(submission_service),
        std::move(source_root_path),
        std::move(input_path),
        std::move(answer_path),
        std::move(*cpp_compiler_path_exp),
        std::move(*python_path_exp),
        std::move(*java_runtime_path_exp)
    );
}

judge_worker::judge_worker(
    submission_service submission_service,
    std::filesystem::path source_root_path,
    std::filesystem::path input_path,
    std::filesystem::path answer_path,
    std::string cpp_compiler_path,
    std::string python_path,
    std::string java_runtime_path
) :
    submission_service_(std::move(submission_service)),
    source_root_path_(std::move(source_root_path)),
    input_path_(std::move(input_path)),
    answer_path_(std::move(answer_path)),
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
            auto run_source_code_exp = run_source_code(save_source_code_exp->value());
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

std::expected<std::optional<std::filesystem::path>, error_code> judge_worker::save_source_code(){
    auto pop_submission_exp = submission_service_.pop_submission();
    if(!pop_submission_exp){
        if(is_queue_empty_error(pop_submission_exp.error())){
            return std::optional<std::filesystem::path>{};
        }

        return std::unexpected(pop_submission_exp.error());
    }

    const auto source_file_path = file_utility::make_source_file_path(
        source_root_path_,
        pop_submission_exp->submission_id,
        pop_submission_exp->language
    );
    
    auto create_file_exp = file_utility::create_file(
        source_file_path,
        pop_submission_exp->source_code
    );

    if(!create_file_exp){
        return std::unexpected(create_file_exp.error());
    }

    return source_file_path;
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
