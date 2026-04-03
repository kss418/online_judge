#include "judge_core/sandbox_runner.hpp"

#include "common/blocking_io.hpp"
#include "common/logger.hpp"
#include "common/temp_file.hpp"
#include "common/unique_fd.hpp"
#include "judge_core/judge_util.hpp"
#include "judge_core/nsjail_util.hpp"

#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <optional>
#include <utility>

namespace{
    struct scratch_files{
        temp_file stdout_temp;
        temp_file stderr_temp;
    };

    std::expected<void, sandbox_error> reset_scratch_file(temp_file& scratch_file){
        if(::ftruncate(scratch_file.get_fd(), 0) < 0){
            return std::unexpected(sandbox_error::from_errno(errno));
        }

        if(::lseek(scratch_file.get_fd(), 0, SEEK_SET) < 0){
            return std::unexpected(sandbox_error::from_errno(errno));
        }

        return {};
    }

    std::expected<scratch_files*, sandbox_error> acquire_thread_local_scratch_files(){
        thread_local std::optional<scratch_files> scratch_files_opt;

        if(!scratch_files_opt.has_value()){
            auto stdout_temp_exp = temp_file::create("/tmp/oj_stdout_XXXXXX");
            if(!stdout_temp_exp){
                return std::unexpected(sandbox_error{stdout_temp_exp.error()});
            }

            auto stderr_temp_exp = temp_file::create("/tmp/oj_stderr_XXXXXX");
            if(!stderr_temp_exp){
                return std::unexpected(sandbox_error{stderr_temp_exp.error()});
            }

            scratch_files_opt.emplace(
                scratch_files{
                    std::move(*stdout_temp_exp),
                    std::move(*stderr_temp_exp)
                }
            );
        }

        const auto reset_stdout_exp = reset_scratch_file(scratch_files_opt->stdout_temp);
        if(!reset_stdout_exp){
            return std::unexpected(reset_stdout_exp.error());
        }

        const auto reset_stderr_exp = reset_scratch_file(scratch_files_opt->stderr_temp);
        if(!reset_stderr_exp){
            return std::unexpected(reset_stderr_exp.error());
        }

        return &*scratch_files_opt;
    }
}

std::expected<void, sandbox_error> sandbox_runner::startup_self_check(){
    const auto nsjail_path_exp = nsjail_util::require_nsjail_path();
    if(!nsjail_path_exp){
        return std::unexpected(nsjail_path_exp.error());
    }
    (void)nsjail_path_exp;

    const auto user_namespace_check_exp = nsjail_util::check_user_namespace_support();
    if(!user_namespace_check_exp){
        return std::unexpected(user_namespace_check_exp.error());
    }

    auto workspace_exp = temp_dir::create("/tmp/oj_sandbox_check_XXXXXX");
    if(!workspace_exp){
        return std::unexpected(sandbox_error{workspace_exp.error()});
    }

    run_options run_options_value;
    run_options_value.workspace_host_path = workspace_exp->get_path();
    run_options_value.time_limit = std::chrono::milliseconds{1000};
    run_options_value.memory_limit_mb = 128;
    run_options_value.policy = policy_profile::run;

    const auto run_exp = run({"/usr/bin/true"}, run_options_value);
    if(!run_exp){
        return std::unexpected(run_exp.error());
    }
    if(!run_exp->is_success()){
        if(!run_exp->stderr_text_.empty()){
            logger::cerr()
                .log("sandbox_startup_self_check_stderr")
                .field("stderr", run_exp->stderr_text_);
        }
        return std::unexpected(sandbox_error{
            sandbox_error_code::unsupported,
            "sandbox startup self check failed"
        });
    }

    return {};
}

void sandbox_runner::invalidate_cached_artifacts() noexcept{
    nsjail_util::invalidate_all_sandbox_artifacts();
}

void sandbox_runner::exec_child(
    const std::vector<std::string>& sandbox_command_args,
    int input_fd,
    int output_fd,
    int error_fd
){
    if(::setpgid(0, 0) < 0){
        _exit(127);
    }

    if(::dup2(input_fd, STDIN_FILENO) < 0) _exit(127);
    if(::dup2(output_fd, STDOUT_FILENO) < 0) _exit(127);
    if(::dup2(error_fd, STDERR_FILENO) < 0) _exit(127);

    ::close(input_fd);
    ::close(output_fd);
    ::close(error_fd);

    if(sandbox_command_args.empty()){
        _exit(127);
    }

    std::vector<char*> argv;
    argv.reserve(sandbox_command_args.size() + 1);
    for(const auto& command_arg : sandbox_command_args){
        argv.push_back(const_cast<char*>(command_arg.c_str()));
    }
    argv.push_back(nullptr);

    ::execv(sandbox_command_args.front().c_str(), argv.data());
    _exit(127);
}

std::expected<sandbox_runner::wait_result, sandbox_error> sandbox_runner::wait_wall_clock(
    pid_t pid, std::chrono::milliseconds time_limit
){
    wait_result out{};
    const auto started_at = std::chrono::steady_clock::now();

    while(true){
        const pid_t wait_result = ::wait4(pid, &out.status_, WNOHANG, &out.usage_);
        if(wait_result == pid){
            break;
        }

        if(wait_result == 0){
            if(time_limit.count() > 0){
                const auto now = std::chrono::steady_clock::now();
                const auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - started_at);
                if(elapsed > time_limit && !out.killed_by_wall_clock_){
                    if(::killpg(pid, SIGKILL) == 0 || errno == ESRCH){
                        out.killed_by_wall_clock_ = true;
                    }
                    else{
                        return std::unexpected(sandbox_error::from_errno(errno));
                    }
                }
            }

            ::usleep(1000);
            continue;
        }

        if(errno == EINTR){
            continue;
        }

        return std::unexpected(sandbox_error{
            sandbox_error_code::internal,
            "waitpid failed"
        });
    }

    const auto finished_at = std::chrono::steady_clock::now();
    out.elapsed_ms_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(finished_at - started_at).count();
    return out;
}

std::expected<sandbox_runner::run_result, sandbox_error> sandbox_runner::run(
    const std::vector<std::string>& command_args,
    const run_options& run_options_value
){
    if(command_args.empty()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    const auto nsjail_path_exp = nsjail_util::require_nsjail_path();
    if(!nsjail_path_exp){
        return std::unexpected(nsjail_path_exp.error());
    }
    (void)nsjail_path_exp;

    const auto validate_workspace_exp = nsjail_util::validate_workspace_path(
        run_options_value.workspace_host_path
    );
    if(!validate_workspace_exp){
        return std::unexpected(validate_workspace_exp.error());
    }

    if(run_options_value.time_limit <= std::chrono::milliseconds::zero()){
        return std::unexpected(sandbox_error{
            sandbox_error_code::invalid_argument,
            "invalid time limit"
        });
    }

    if(run_options_value.memory_limit_mb <= 0){
        return std::unexpected(sandbox_error{
            sandbox_error_code::invalid_argument,
            "invalid memory limit"
        });
    }

    auto scratch_files_exp = acquire_thread_local_scratch_files();
    if(!scratch_files_exp){
        return std::unexpected(scratch_files_exp.error());
    }
    scratch_files& scratch_files_value = **scratch_files_exp;

    auto sandbox_artifacts_exp = nsjail_util::acquire_sandbox_artifacts(
        run_options_value.policy
    );
    if(!sandbox_artifacts_exp){
        return std::unexpected(sandbox_artifacts_exp.error());
    }

    const auto sandbox_command_args_exp = nsjail_util::make_command_args(
        *nsjail_path_exp,
        **sandbox_artifacts_exp,
        command_args,
        run_options_value
    );
    if(!sandbox_command_args_exp){
        return std::unexpected(sandbox_command_args_exp.error());
    }

    const char* input_path_string =
        run_options_value.input_path_opt
            ? run_options_value.input_path_opt->c_str()
            : "/dev/null";
    unique_fd input_fd(::open(input_path_string, O_RDONLY));
    if(!input_fd){
        return std::unexpected(sandbox_error::from_errno(errno));
    }

    const pid_t pid = ::fork();
    if(pid < 0){
        return std::unexpected(sandbox_error{
            sandbox_error_code::internal,
            "fork failed"
        });
    }

    if(pid == 0){
        exec_child(
            *sandbox_command_args_exp,
            input_fd.get(),
            scratch_files_value.stdout_temp.get_fd(),
            scratch_files_value.stderr_temp.get_fd()
        );
    }
    else if(::setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH){
        return std::unexpected(sandbox_error::from_errno(errno));
    }

    input_fd.close();

    auto wait_exp = wait_wall_clock(pid, run_options_value.time_limit);
    if(!wait_exp){
        return std::unexpected(wait_exp.error());
    }
    auto wait_result = std::move(*wait_exp);

    auto stdout_text_exp = blocking_io::read_all_from_start(
        scratch_files_value.stdout_temp.get_fd()
    );
    if(!stdout_text_exp){
        return std::unexpected(sandbox_error{stdout_text_exp.error()});
    }

    auto stderr_text_exp = blocking_io::read_all_from_start(
        scratch_files_value.stderr_temp.get_fd()
    );
    if(!stderr_text_exp){
        return std::unexpected(sandbox_error{stderr_text_exp.error()});
    }

    run_result result;
    result.stderr_text_ = std::move(*stderr_text_exp);
    result.max_rss_kb_ = static_cast<std::size_t>(wait_result.usage_.ru_maxrss);
    result.elapsed_ms_ = wait_result.elapsed_ms_;

    const std::size_t memory_limit_kb =
        static_cast<std::size_t>(run_options_value.memory_limit_mb) * 1024ULL;
    if(result.max_rss_kb_ > memory_limit_kb){
        result.memory_limit_exceeded_ = true;
    }

    result.output_lines_ = judge_util::instance().normalize_output(*stdout_text_exp);

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
#ifdef SIGXFSZ
        if(signal_number == SIGXFSZ){
            result.output_exceeded_ = true;
        }
#endif
        if(!result.time_limit_exceeded_ && !result.output_exceeded_){
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

    return std::unexpected(sandbox_error{
        sandbox_error_code::internal,
        "waitpid failed"
    });
}
