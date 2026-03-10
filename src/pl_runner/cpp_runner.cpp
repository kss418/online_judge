#include "pl_runner/cpp_runner.hpp"

#include "common/unique_fd.hpp"

#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>

std::expected<cpp_runner::compile_result, error_code> cpp_runner::compile(
    const path& source_file_path,
    const path& compiler_path
){
    auto binary_file_exp = temp_file::create("/tmp/oj_binary_XXXXXX");
    if(!binary_file_exp){
        return std::unexpected(binary_file_exp.error());
    }

    int stderr_pipe[2];
    if(pipe(stderr_pipe) < 0){
        return std::unexpected(error_code::create(syscall_error::pipe_failed));
    }

    unique_fd read_fd = unique_fd(stderr_pipe[0]);
    unique_fd write_fd = unique_fd(stderr_pipe[1]);
    const pid_t pid = fork();
    if(pid < 0){
        return std::unexpected(error_code::create(syscall_error::fork_failed));
    }

    if(pid == 0){
        dup2(write_fd.get(), STDERR_FILENO);
        write_fd.close();
        read_fd.close();

        std::vector<char*> argv = {
            const_cast<char*>(compiler_path.c_str()),
            const_cast<char*>("-std=c++23"),
            const_cast<char*>("-O2"),
            const_cast<char*>("-pipe"),
            const_cast<char*>(source_file_path.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(binary_file_exp->get_path().c_str()),
            nullptr
        };

        execv(compiler_path.c_str(), argv.data());
        _exit(127);
    }

    write_fd.close();
    std::string stderr_text;
    char buffer[4096];
    while(true){
        const ssize_t read_size = read(read_fd.get(), buffer, sizeof(buffer));
        if(read_size > 0){
            stderr_text.append(buffer, static_cast<std::size_t>(read_size));
            continue;
        }

        if(read_size == 0){
            break;
        }

        const int error_number = errno;
        if(error_number == EINTR){
            continue;
        }

        return std::unexpected(error_code::create(error_code::map_errno(error_number)));
    }
    read_fd.close();

    int status = 0;
    while(true){
        if(waitpid(pid, &status, 0) == pid){
            break;
        }

        const int error_number = errno;
        if(error_number == EINTR){
            continue;
        }

        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    if(WIFSIGNALED(status)){
        return std::unexpected(error_code::create(error_code::map_signal(WTERMSIG(status))));
    }

    if(!WIFEXITED(status)){
        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    compile_result compile_result_value;
    compile_result_value.binary_file_ = std::move(*binary_file_exp);
    compile_result_value.binary_file_.close_fd();
    compile_result_value.run_command_args_.push_back(
        compile_result_value.binary_file_.get_path().string()
    );
    compile_result_value.exit_code_ = WEXITSTATUS(status);
    compile_result_value.stderr_text_ = std::move(stderr_text);
    return compile_result_value;
}

std::expected<pl_runner_utility::prepared_source, error_code> cpp_runner::prepare(
    const path& source_file_path,
    const path& compiler_path
){
    auto compile_cpp_exp = compile(source_file_path, compiler_path);
    if(!compile_cpp_exp){
        return std::unexpected(compile_cpp_exp.error());
    }

    if(!compile_cpp_exp->is_success()){
        return pl_runner_utility::make_compile_failed_prepared_source(
            compile_cpp_exp->exit_code_,
            std::move(compile_cpp_exp->stderr_text_)
        );
    }

    pl_runner_utility::prepared_source prepared_source_value;
    prepared_source_value.binary_file_ = std::move(compile_cpp_exp->binary_file_);
    prepared_source_value.run_command_args_ = std::move(compile_cpp_exp->run_command_args_);
    return prepared_source_value;
}
