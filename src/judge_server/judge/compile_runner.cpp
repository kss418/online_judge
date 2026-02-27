#include "judge_server/judge/compile_runner.hpp"
#include "judge_server/core/unique_fd.hpp"

#include <cerrno>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

std::expected <compile_runner::compile_result, error_code> compile_runner::compile_cpp(
    const path& source_path, const path& output_path, const path& compiler_path
){
    int stderr_pipe[2];
    if(pipe(stderr_pipe) < 0){
        return std::unexpected(error_code::create(syscall_error::pipe_failed));
    }

    unique_fd read_fd = unique_fd(stderr_pipe[0]), write_fd = unique_fd(stderr_pipe[1]);
    pid_t pid = fork();

    if(pid < 0){
        return std::unexpected(error_code::create(syscall_error::fork_failed));
    }
    
    // child process
    if(pid == 0){
        dup2(write_fd.get(), STDERR_FILENO);
        write_fd.close();
        read_fd.close();

        std::vector <char*> argv = {
            const_cast<char*>(compiler_path.c_str()),
            const_cast<char*>("-std=c++23"),
            const_cast<char*>("-O2"),
            const_cast<char*>("-pipe"),
            const_cast<char*>(source_path.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(output_path.c_str()),
            nullptr
        };

        execv(compiler_path.c_str(), argv.data());
        _exit(127);
    }

    // parent process
    write_fd.close();
    std::string stderr_text;
    char buf[4096];

    while (true){
        ssize_t n = read(read_fd.get(), buf, sizeof(buf));
        if(n > 0){
            stderr_text.append(buf, static_cast<size_t>(n));
        }
        else if(n == 0){
            break;
        } 
        else{
            int ec = errno;
            if(ec == EINTR) continue;
            return std::unexpected(error_code::create(error_code::map_errno(ec)));
        }
    }
    read_fd.close();

    int status = 0;
    while(true){
        if(waitpid(pid, &status, 0) == pid) break;
        int ec = errno;
        if(ec == EINTR) continue;
        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    if(WIFEXITED(status)){
        int exit_code = WEXITSTATUS(status);
        return compile_result(exit_code, stderr_text);
    }
    
    if(WIFSIGNALED(status)){
        int signal_number = WTERMSIG(status);
        return std::unexpected(error_code::create(error_code::map_signal(signal_number)));
    }

    return std::unexpected(error_code::create(syscall_error::waitpid_failed));
}
