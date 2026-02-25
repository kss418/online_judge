#include "judge/code_runner.hpp"
#include "core/temp_file.hpp"
#include "net/blocking_io.hpp"
#include "judge/judge_utility.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::expected<code_runner::run_result, error_code> code_runner::run_cpp(path binary_path, path input_path){
    auto stdout_temp = temp_file::create("/tmp/oj_stdout_XXXXXX");
    if(!stdout_temp){
        return std::unexpected(stdout_temp.error());
    }

    auto stderr_temp = temp_file::create("/tmp/oj_stderr_XXXXXX");
    if(!stderr_temp){
        return std::unexpected(stderr_temp.error());
    }

    unique_fd input_fd = unique_fd(open(input_path.c_str(), O_RDONLY));
    if(!input_fd){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    pid_t pid = fork();
    if(pid < 0){
        return std::unexpected(error_code::create(syscall_error::fork_failed));
    }
    
    // child process
    if(pid == 0){
        if(dup2(input_fd.get(), STDIN_FILENO) < 0) _exit(127);
        if(dup2(stdout_temp->get_fd(), STDOUT_FILENO) < 0) _exit(127);
        if(dup2(stderr_temp->get_fd(), STDERR_FILENO) < 0) _exit(127);

        input_fd.close();
        stdout_temp->close_fd();
        stderr_temp->close_fd();

        std::vector <char*> argv = {
            const_cast<char*>(binary_path.c_str()),
            nullptr
        };

        execv(binary_path.c_str(), argv.data());
        _exit(127);
    }

    // parent process
    input_fd.close();

    int status = 0;
    while(true){
        if(waitpid(pid, &status, 0) == pid) break;
        int ec = errno;
        if(ec == EINTR) continue;
        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    if(lseek(stdout_temp->get_fd(), 0, SEEK_SET) < 0){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    if(lseek(stderr_temp->get_fd(), 0, SEEK_SET) < 0){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    auto stdout_text_exp = blocking_io::read_all(stdout_temp->get_fd());
    if(!stdout_text_exp){
        return std::unexpected(stdout_text_exp.error());
    }

    auto stderr_text_exp = blocking_io::read_all(stderr_temp->get_fd());
    if(!stderr_text_exp){
        return std::unexpected(stderr_text_exp.error());
    }

    run_result result;
    result.output_lines_ = judge_utility::normalize_output(*stdout_text_exp);
    result.stderr_text_ = std::move(*stderr_text_exp);

    if(WIFEXITED(status)){
        result.exit_code_ = WEXITSTATUS(status);
        return result;
    }
    
    if(WIFSIGNALED(status)){
        int signal_number = WTERMSIG(status);
        result.exit_code_ = 128 + signal_number;
        return result;
    }

    return std::unexpected(error_code::create(syscall_error::waitpid_failed));
}
