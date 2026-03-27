#include "judge_core/sandbox_runner.hpp"

#include "common/blocking_io.hpp"
#include "common/env_util.hpp"
#include "common/file_util.hpp"
#include "common/temp_dir.hpp"
#include "common/temp_file.hpp"
#include "common/unique_fd.hpp"
#include "judge_core/judge_util.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace{
    constexpr std::string_view SANDBOX_WORKSPACE_PATH = "/workspace";
    constexpr rlim_t SANDBOX_NOFILE_LIMIT = 64;
    constexpr rlim_t COMPILE_NPROC_LIMIT = 256;
    constexpr rlim_t DEFAULT_RUN_NPROC_LIMIT = 64;
    constexpr rlim_t JAVA_RUN_NPROC_LIMIT = 512;
    constexpr std::int64_t OUTPUT_FILE_LIMIT_MB = 8;

    std::expected<std::filesystem::path, error_code> require_nsjail_path(){
        const auto nsjail_path_exp = env_util::require_env("JUDGE_NSJAIL_PATH");
        if(!nsjail_path_exp){
            return std::unexpected(nsjail_path_exp.error());
        }

        const std::filesystem::path nsjail_path = *nsjail_path_exp;
        if(::access(nsjail_path.c_str(), X_OK) < 0){
            return std::unexpected(error_code::create(error_code::map_errno(errno)));
        }

        return nsjail_path;
    }

    std::expected<void, error_code> validate_workspace_path(
        const std::filesystem::path& workspace_host_path
    ){
        if(workspace_host_path.empty() || !workspace_host_path.is_absolute()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        std::error_code exists_ec;
        const bool exists_value = std::filesystem::exists(workspace_host_path, exists_ec);
        if(exists_ec){
            return std::unexpected(error_code::create(error_code::map_errno(exists_ec.value())));
        }
        if(!exists_value){
            return std::unexpected(error_code::create(errno_error::file_not_found));
        }

        return {};
    }

    std::expected<void, error_code> check_user_namespace_support(){
        int error_pipe[2];
        if(::pipe(error_pipe) < 0){
            return std::unexpected(error_code::create(syscall_error::pipe_failed));
        }

        unique_fd read_fd(error_pipe[0]);
        unique_fd write_fd(error_pipe[1]);

        const pid_t pid = ::fork();
        if(pid < 0){
            return std::unexpected(error_code::create(syscall_error::fork_failed));
        }

        if(pid == 0){
            read_fd.close();

            int error_number = 0;
            if(::unshare(CLONE_NEWUSER) < 0){
                error_number = errno;
            }

            const ssize_t written_size = ::write(
                write_fd.get(),
                &error_number,
                sizeof(error_number)
            );
            (void)written_size;
            _exit(error_number == 0 ? 0 : 1);
        }

        write_fd.close();

        int wait_status = 0;
        while(true){
            if(::waitpid(pid, &wait_status, 0) == pid){
                break;
            }

            if(errno == EINTR){
                continue;
            }

            return std::unexpected(error_code::create(syscall_error::waitpid_failed));
        }

        int error_number = 0;
        const auto read_error_exp = blocking_io::read_all(read_fd.get());
        if(!read_error_exp){
            return std::unexpected(read_error_exp.error());
        }
        if(read_error_exp->size() == sizeof(error_number)){
            std::memcpy(&error_number, read_error_exp->data(), sizeof(error_number));
        }

        if(WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 0){
            return {};
        }

        if(error_number != 0){
            return std::unexpected(error_code::create(error_code::map_errno(error_number)));
        }

        return std::unexpected(error_code::create(errno_error::operation_not_supported));
    }

    std::string join_syscall_names(std::initializer_list<std::string_view> syscalls){
        std::string joined;
        bool is_first = true;

        for(const std::string_view syscall_name : syscalls){
            if(!is_first){
                joined += ", ";
            }
            joined.append(syscall_name);
            is_first = false;
        }

        return joined;
    }

    std::string make_seccomp_policy_text(sandbox_runner::policy_profile policy_profile_value){
        static constexpr std::initializer_list<std::string_view> common_blocked_syscalls = {
            "socket",
            "socketpair",
            "connect",
            "accept",
            "accept4",
            "bind",
            "listen",
            "setns",
            "unshare",
            "mount",
            "ptrace",
            "seccomp"
        };
        static constexpr std::initializer_list<std::string_view> runtime_extra_blocked_syscalls = {
            "clone",
            "clone3",
            "fork",
            "vfork",
            "pipe",
            "pipe2",
            "dup",
            "dup2",
            "dup3",
            "eventfd",
            "eventfd2",
            "timerfd_create",
            "signalfd",
            "signalfd4",
            "inotify_init",
            "inotify_init1",
            "memfd_create",
            "userfaultfd"
        };

        std::string blocked_syscalls = join_syscall_names(common_blocked_syscalls);
        if(policy_profile_value == sandbox_runner::policy_profile::run){
            blocked_syscalls += ", ";
            blocked_syscalls += join_syscall_names(runtime_extra_blocked_syscalls);
        }

        std::string policy_text;
        policy_text.reserve(blocked_syscalls.size() + 80);
        policy_text += "POLICY sandbox {\n";
        policy_text += "  KILL { ";
        policy_text += blocked_syscalls;
        policy_text += " }\n";
        policy_text += "}\n";
        policy_text += "USE sandbox DEFAULT ALLOW\n";
        return policy_text;
    }

    std::expected<void, error_code> create_empty_file(const std::filesystem::path& file_path){
        unique_fd file_fd(::open(
            file_path.c_str(),
            O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC,
            0644
        ));
        if(!file_fd){
            return std::unexpected(error_code::create(error_code::map_errno(errno)));
        }

        return {};
    }

    std::expected<void, error_code> ensure_mount_target(
        const std::filesystem::path& rootfs_path,
        const std::filesystem::path& target_path,
        const std::filesystem::path& source_path
    ){
        const std::filesystem::path relative_target = target_path.is_absolute()
            ? target_path.lexically_relative("/")
            : target_path;
        if(relative_target.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        const std::filesystem::path full_target_path = rootfs_path / relative_target;
        std::error_code type_ec;
        const bool is_directory = std::filesystem::is_directory(source_path, type_ec);
        if(type_ec){
            return std::unexpected(error_code::create(error_code::map_errno(type_ec.value())));
        }

        if(is_directory){
            return file_util::create_directories(full_target_path);
        }

        const auto create_parent_dirs_exp = file_util::create_directories(
            full_target_path.parent_path()
        );
        if(!create_parent_dirs_exp){
            return std::unexpected(create_parent_dirs_exp.error());
        }

        return create_empty_file(full_target_path);
    }

    std::expected<void, error_code> append_mount_if_exists(
        std::vector<std::string>& sandbox_command_args,
        const std::filesystem::path& rootfs_path,
        const std::filesystem::path& source_path,
        const std::filesystem::path& target_path,
        bool writable
    ){
        std::error_code exists_ec;
        const bool exists_value = std::filesystem::exists(source_path, exists_ec);
        if(exists_ec){
            return std::unexpected(error_code::create(error_code::map_errno(exists_ec.value())));
        }
        if(!exists_value){
            return {};
        }

        const auto ensure_mount_target_exp = ensure_mount_target(
            rootfs_path,
            target_path,
            source_path
        );
        if(!ensure_mount_target_exp){
            return std::unexpected(ensure_mount_target_exp.error());
        }

        sandbox_command_args.push_back(writable ? "-B" : "-R");
        sandbox_command_args.push_back(source_path.string() + ":" + target_path.string());
        return {};
    }

    std::expected<std::filesystem::path, error_code> prepare_rootfs_layout(
        const temp_dir& sandbox_dir
    ){
        const std::filesystem::path rootfs_path = sandbox_dir.get_path() / "rootfs";
        const auto create_rootfs_exp = file_util::create_directories(rootfs_path);
        if(!create_rootfs_exp){
            return std::unexpected(create_rootfs_exp.error());
        }

        for(const std::filesystem::path& relative_dir : {
                std::filesystem::path{"workspace"},
                std::filesystem::path{"tmp"},
                std::filesystem::path{"proc"},
                std::filesystem::path{"dev"}
            }){
            const auto create_directory_exp = file_util::create_directories(rootfs_path / relative_dir);
            if(!create_directory_exp){
                return std::unexpected(create_directory_exp.error());
            }
        }

        return rootfs_path;
    }

    std::expected<std::filesystem::path, error_code> create_seccomp_policy_file(
        const temp_dir& sandbox_dir,
        sandbox_runner::policy_profile policy_profile_value
    ){
        const std::filesystem::path policy_path = sandbox_dir.get_path() / "policy.kafel";
        const auto write_policy_exp = file_util::create_file(
            policy_path,
            make_seccomp_policy_text(policy_profile_value)
        );
        if(!write_policy_exp){
            return std::unexpected(write_policy_exp.error());
        }

        return policy_path;
    }

    std::int64_t make_process_limit(const sandbox_runner::run_options& run_options_value){
        if(run_options_value.policy == sandbox_runner::policy_profile::compile){
            return COMPILE_NPROC_LIMIT;
        }

        if(run_options_value.mounts == sandbox_runner::mount_profile::java_profile){
            return JAVA_RUN_NPROC_LIMIT;
        }

        return DEFAULT_RUN_NPROC_LIMIT;
    }

    std::expected<std::vector<std::string>, error_code> make_nsjail_command_args(
        const std::filesystem::path& nsjail_path,
        const std::filesystem::path& rootfs_path,
        const std::filesystem::path& seccomp_policy_path,
        const std::vector<std::string>& command_args,
        const sandbox_runner::run_options& run_options_value
    ){
        const auto cpu_limit_seconds_value =
            std::max<std::int64_t>(1, (run_options_value.time_limit.count() + 999) / 1000);
        const auto wall_time_limit_seconds_value =
            std::max<std::int64_t>(1, (run_options_value.time_limit.count() + 999) / 1000);

        std::vector<std::string> sandbox_command_args;
        sandbox_command_args.reserve(48 + command_args.size());
        sandbox_command_args.push_back(nsjail_path.string());
        sandbox_command_args.push_back("-Me");
        sandbox_command_args.push_back("-Q");
        sandbox_command_args.push_back("--disable_clone_newcgroup");
        sandbox_command_args.push_back("-c");
        sandbox_command_args.push_back(rootfs_path.string());
        sandbox_command_args.push_back("-D");
        sandbox_command_args.push_back(std::string(SANDBOX_WORKSPACE_PATH));
        sandbox_command_args.push_back("-E");
        sandbox_command_args.push_back("HOME=/tmp");
        sandbox_command_args.push_back("-E");
        sandbox_command_args.push_back("TMPDIR=/tmp");
        sandbox_command_args.push_back("-E");
        sandbox_command_args.push_back("PATH=/usr/bin:/bin");
        sandbox_command_args.push_back("-t");
        sandbox_command_args.push_back(std::to_string(wall_time_limit_seconds_value));
        sandbox_command_args.push_back("--rlimit_as");
        sandbox_command_args.push_back(std::to_string(run_options_value.memory_limit_mb));
        sandbox_command_args.push_back("--rlimit_cpu");
        sandbox_command_args.push_back(std::to_string(cpu_limit_seconds_value));
        sandbox_command_args.push_back("--rlimit_nofile");
        sandbox_command_args.push_back(std::to_string(SANDBOX_NOFILE_LIMIT));
        sandbox_command_args.push_back("--rlimit_fsize");
        sandbox_command_args.push_back(std::to_string(OUTPUT_FILE_LIMIT_MB));
        sandbox_command_args.push_back("--rlimit_core");
        sandbox_command_args.push_back("0");
        sandbox_command_args.push_back("--rlimit_nproc");
        sandbox_command_args.push_back(std::to_string(make_process_limit(run_options_value)));
        sandbox_command_args.push_back("-P");
        sandbox_command_args.push_back(seccomp_policy_path.string());

        for(const std::filesystem::path& ro_mount : {
                std::filesystem::path{"/usr"},
                std::filesystem::path{"/bin"},
                std::filesystem::path{"/sbin"},
                std::filesystem::path{"/lib"},
                std::filesystem::path{"/lib64"},
                std::filesystem::path{"/etc"}
            }){
            const auto append_mount_exp = append_mount_if_exists(
                sandbox_command_args,
                rootfs_path,
                ro_mount,
                ro_mount,
                false
            );
            if(!append_mount_exp){
                return std::unexpected(append_mount_exp.error());
            }
        }

        for(const std::filesystem::path& device_path : {
                std::filesystem::path{"/dev/null"},
                std::filesystem::path{"/dev/zero"},
                std::filesystem::path{"/dev/random"},
                std::filesystem::path{"/dev/urandom"}
            }){
            const auto append_mount_exp = append_mount_if_exists(
                sandbox_command_args,
                rootfs_path,
                device_path,
                device_path,
                false
            );
            if(!append_mount_exp){
                return std::unexpected(append_mount_exp.error());
            }
        }

        const auto append_workspace_mount_exp = append_mount_if_exists(
            sandbox_command_args,
            rootfs_path,
            run_options_value.workspace_host_path,
            SANDBOX_WORKSPACE_PATH,
            true
        );
        if(!append_workspace_mount_exp){
            return std::unexpected(append_workspace_mount_exp.error());
        }

        sandbox_command_args.push_back("-T");
        sandbox_command_args.push_back("/tmp");
        sandbox_command_args.push_back("--");

        for(const auto& command_arg : command_args){
            sandbox_command_args.push_back(command_arg);
        }

        return sandbox_command_args;
    }
}

std::expected<void, error_code> sandbox_runner::startup_self_check(){
    const auto nsjail_path_exp = require_nsjail_path();
    if(!nsjail_path_exp){
        return std::unexpected(nsjail_path_exp.error());
    }
    (void)nsjail_path_exp;

    const auto user_namespace_check_exp = check_user_namespace_support();
    if(!user_namespace_check_exp){
        return std::unexpected(user_namespace_check_exp.error());
    }

    auto workspace_exp = temp_dir::create("/tmp/oj_sandbox_check_XXXXXX");
    if(!workspace_exp){
        return std::unexpected(workspace_exp.error());
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
            std::cerr << "sandbox self-check stderr: " << run_exp->stderr_text_ << '\n';
        }
        return std::unexpected(error_code::create(errno_error::operation_not_supported));
    }

    return {};
}

void sandbox_runner::exec_child(
    const std::vector<std::string>& command_args,
    int input_fd,
    int output_fd,
    int error_fd,
    const path& rootfs_path,
    const path& seccomp_policy_path,
    const run_options& run_options_value
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

    const auto nsjail_path_exp = require_nsjail_path();
    if(!nsjail_path_exp){
        _exit(127);
    }

    const auto sandbox_command_args_exp = make_nsjail_command_args(
        *nsjail_path_exp,
        rootfs_path,
        seccomp_policy_path,
        command_args,
        run_options_value
    );
    if(!sandbox_command_args_exp){
        _exit(127);
    }

    std::vector<char*> argv;
    argv.reserve(sandbox_command_args_exp->size() + 1);
    for(const auto& command_arg : *sandbox_command_args_exp){
        argv.push_back(const_cast<char*>(command_arg.c_str()));
    }
    argv.push_back(nullptr);

    ::execv(nsjail_path_exp->c_str(), argv.data());
    _exit(127);
}

std::expected<sandbox_runner::wait_result, error_code> sandbox_runner::wait_wall_clock(
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
                        return std::unexpected(error_code::create(error_code::map_errno(errno)));
                    }
                }
            }

            ::usleep(1000);
            continue;
        }

        if(errno == EINTR){
            continue;
        }

        return std::unexpected(error_code::create(syscall_error::waitpid_failed));
    }

    const auto finished_at = std::chrono::steady_clock::now();
    out.elapsed_ms_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(finished_at - started_at).count();
    return out;
}

std::expected<sandbox_runner::run_result, error_code> sandbox_runner::run(
    const std::vector<std::string>& command_args,
    const run_options& run_options_value
){
    if(command_args.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto nsjail_path_exp = require_nsjail_path();
    if(!nsjail_path_exp){
        return std::unexpected(nsjail_path_exp.error());
    }
    (void)nsjail_path_exp;

    const auto validate_workspace_exp = validate_workspace_path(
        run_options_value.workspace_host_path
    );
    if(!validate_workspace_exp){
        return std::unexpected(validate_workspace_exp.error());
    }

    if(run_options_value.time_limit <= std::chrono::milliseconds::zero()){
        return std::unexpected(error_code::create(limit_error::invalid_time_limit));
    }

    if(run_options_value.memory_limit_mb <= 0){
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

    auto sandbox_dir_exp = temp_dir::create("/tmp/oj_nsjail_XXXXXX");
    if(!sandbox_dir_exp){
        return std::unexpected(sandbox_dir_exp.error());
    }
    temp_dir sandbox_dir = std::move(*sandbox_dir_exp);

    const auto rootfs_path_exp = prepare_rootfs_layout(sandbox_dir);
    if(!rootfs_path_exp){
        return std::unexpected(rootfs_path_exp.error());
    }

    const auto seccomp_policy_path_exp = create_seccomp_policy_file(
        sandbox_dir,
        run_options_value.policy
    );
    if(!seccomp_policy_path_exp){
        return std::unexpected(seccomp_policy_path_exp.error());
    }

    const char* input_path_string =
        run_options_value.input_path_opt
            ? run_options_value.input_path_opt->c_str()
            : "/dev/null";
    unique_fd input_fd(::open(input_path_string, O_RDONLY));
    if(!input_fd){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    const pid_t pid = ::fork();
    if(pid < 0){
        return std::unexpected(error_code::create(syscall_error::fork_failed));
    }

    if(pid == 0){
        exec_child(
            command_args,
            input_fd.get(),
            stdout_temp->get_fd(),
            stderr_temp->get_fd(),
            *rootfs_path_exp,
            *seccomp_policy_path_exp,
            run_options_value
        );
    }
    else if(::setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    input_fd.close();

    auto wait_exp = wait_wall_clock(pid, run_options_value.time_limit);
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

    return std::unexpected(error_code::create(syscall_error::waitpid_failed));
}
