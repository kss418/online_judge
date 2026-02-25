#include "judge/code_runner.hpp"
#include "core/temp_file.hpp"
#include "net/blocking_io.hpp"
#include "judge/judge_utility.hpp"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

void code_runner::exec_child(
    const path& binary_path, int input_fd, int output_fd, int error_fd,
    std::chrono::milliseconds time_limit, std::int64_t memory_limit_mb
){
    const auto cpu_limit_seconds_value = (time_limit.count() + 999) / 1000;
    const rlim_t cpu_limit_seconds = static_cast<rlim_t>(cpu_limit_seconds_value);
    const rlimit cpu_limit{cpu_limit_seconds, cpu_limit_seconds};
    if(setrlimit(RLIMIT_CPU, &cpu_limit) < 0) _exit(127);

    const unsigned long long memory_limit_bytes_raw =
        static_cast<unsigned long long>(memory_limit_mb) * 1024ULL * 1024ULL;
    const rlim_t memory_limit_bytes = static_cast<rlim_t>(memory_limit_bytes_raw);
    const rlimit memory_limit{memory_limit_bytes, memory_limit_bytes};
    if(setrlimit(RLIMIT_AS, &memory_limit) < 0) _exit(127);

    if(dup2(input_fd, STDIN_FILENO) < 0) _exit(127);
    if(dup2(output_fd, STDOUT_FILENO) < 0) _exit(127);
    if(dup2(error_fd, STDERR_FILENO) < 0) _exit(127);

    ::close(input_fd);
    ::close(output_fd);
    ::close(error_fd);

    std::vector<char*> argv = {
        const_cast<char*>(binary_path.c_str()),
        nullptr
    };

    execv(binary_path.c_str(), argv.data());
    _exit(127);
}

std::expected <code_runner::wait_result, error_code> code_runner::wait_wall_clock(
    pid_t pid, std::chrono::milliseconds time_limit
){
    wait_result out{};
    const auto started_at = std::chrono::steady_clock::now();

    while(true){
        const pid_t wait_result = wait4(pid, &out.status_, WNOHANG, &out.usage_);
        if(wait_result == pid) break;

        if(wait_result == 0){
            if(time_limit.count() > 0){
                const auto now = std::chrono::steady_clock::now();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - started_at);
                if(elapsed > time_limit && !out.killed_by_wall_clock_){
                    if(kill(pid, SIGKILL) == 0 || errno == ESRCH){
                        out.killed_by_wall_clock_ = true;
                    }
                    else{
                        return std::unexpected(error_code::create(error_code::map_errno(errno)));
                    }
                }
            }

            usleep(1000);
            continue;
        }

        int ec = errno;
        if(ec == EINTR) continue;
        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    const auto finished_at = std::chrono::steady_clock::now();
    out.elapsed_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(finished_at - started_at).count();
    return out;
}

std::expected<code_runner::run_result, error_code> code_runner::run_cpp(
    const path& binary_path, const path& input_path, std::chrono::milliseconds time_limit, std::int64_t memory_limit_mb
){
    if(time_limit <= std::chrono::milliseconds::zero()){
        return std::unexpected(error_code::create(limit_error::invalid_time_limit));
    }

    if(memory_limit_mb <= 0){
        return std::unexpected(error_code::create(limit_error::invalid_memory_limit));
    }

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

    if(pid == 0){
        exec_child(
            binary_path, input_fd.get(), stdout_temp->get_fd(), stderr_temp->get_fd(), 
            time_limit, memory_limit_mb
        );
    }

    input_fd.close();
    auto wait_exp = wait_wall_clock(pid, time_limit);
    if(!wait_exp){
        return std::unexpected(wait_exp.error());
    }
    auto wait_result = std::move(*wait_exp);

    auto stdout_text_exp = blocking_io::read_all_from_start(stdout_temp->get_fd());
    if(!stdout_text_exp){
        return std::unexpected(stdout_text_exp.error());
    }

    auto stderr_text_exp = blocking_io::read_all_from_start(stderr_temp->get_fd());
    if(!stderr_text_exp){
        return std::unexpected(stderr_text_exp.error());
    }

    run_result result;
    result.output_lines_ = judge_utility::normalize_output(*stdout_text_exp);
    result.stderr_text_ = std::move(*stderr_text_exp);
    result.max_rss_kb_ = static_cast<std::size_t>(wait_result.usage_.ru_maxrss);
    result.elapsed_ms_ = wait_result.elapsed_ms_;

    const std::size_t memory_limit_kb = static_cast<std::size_t>(memory_limit_mb) * 1024ULL;
    if(result.max_rss_kb_ > memory_limit_kb){
        result.memory_limit_exceeded_ = true;
    }

    if(WIFEXITED(wait_result.status_)){
        result.exit_code_ = WEXITSTATUS(wait_result.status_);
        if(
            result.exit_code_ != 0 &&
            result.stderr_text_.find("bad_alloc") != std::string::npos
        ){
            result.memory_limit_exceeded_ = true;
        }
        return result;
    }

    if(WIFSIGNALED(wait_result.status_)){
        const int signal_number = WTERMSIG(wait_result.status_);
        result.exit_code_ = 128 + signal_number;

        if(signal_number == SIGXCPU || wait_result.killed_by_wall_clock_){
            result.time_limit_exceeded_ = true;
        }

        if(!result.time_limit_exceeded_){
            if(signal_number == SIGSEGV){
                result.memory_limit_exceeded_ = true;
            }
            else if(
                signal_number == SIGABRT &&
                result.stderr_text_.find("bad_alloc") != std::string::npos
            ){
                result.memory_limit_exceeded_ = true;
            }
        }

        return result;
    }

    return std::unexpected(error_code::create(syscall_error::waitpid_failed));
}
