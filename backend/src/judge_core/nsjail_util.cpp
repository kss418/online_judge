#include "judge_core/nsjail_util.hpp"

#include "common/blocking_io.hpp"
#include "common/env_util.hpp"
#include "common/file_util.hpp"
#include "common/unique_fd.hpp"
#include "error/io_error.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace nsjail_util::detail{
    constexpr std::string_view sandbox_workspace_path = "/workspace";
    constexpr rlim_t sandbox_nofile_limit = 64;
    constexpr rlim_t compile_nproc_limit = 256;
    constexpr rlim_t default_run_nproc_limit = 64;
    constexpr rlim_t java_run_nproc_limit = 512;
    constexpr std::int64_t output_file_limit_mb = 8;
    constexpr std::array<std::string_view, 6> readonly_mount_paths = {
        "/usr",
        "/bin",
        "/sbin",
        "/lib",
        "/lib64",
        "/etc"
    };
    constexpr std::array<std::string_view, 4> readonly_device_paths = {
        "/dev/null",
        "/dev/zero",
        "/dev/random",
        "/dev/urandom"
    };
    sandbox_error make_invalid_configuration(std::string message){
        return sandbox_error{
            sandbox_error_code::invalid_configuration,
            std::move(message)
        };
    }

    sandbox_error make_invalid_configuration(const io_error& error){
        return make_invalid_configuration(error.message);
    }

    sandbox_error make_invalid_configuration(const infra_error& error){
        return make_invalid_configuration(error.message);
    }

    std::string ascii_lowercase(std::string value){
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character){
                return static_cast<char>(std::tolower(character));
            }
        );
        return value;
    }

    bool running_under_wsl(){
        for(const char* env_name : {"WSL_DISTRO_NAME", "WSL_INTEROP"}){
            const char* env_value = getenv(env_name);
            if(env_value != nullptr && *env_value != '\0'){
                return true;
            }
        }

        const auto osrelease_text_exp =
            file_util::read_file_content("/proc/sys/kernel/osrelease");
        if(!osrelease_text_exp){
            return false;
        }

        const std::string normalized_osrelease = ascii_lowercase(*osrelease_text_exp);
        return
            normalized_osrelease.find("microsoft") != std::string::npos ||
            normalized_osrelease.find("wsl") != std::string::npos;
    }

    bool should_disable_network_namespace(){
        return running_under_wsl();
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
            // nsjail 3.4 bundles a kafel parser that does not recognize clone3.
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

    std::expected<void, sandbox_error> create_empty_file(const std::filesystem::path& file_path){
        unique_fd file_fd(::open(
            file_path.c_str(),
            O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC,
            0644
        ));
        if(!file_fd){
            return std::unexpected(sandbox_error::from_errno(errno));
        }

        return {};
    }

    std::expected<void, sandbox_error> ensure_mount_target(
        const std::filesystem::path& rootfs_path,
        const std::filesystem::path& target_path,
        const std::filesystem::path& source_path
    ){
        const std::filesystem::path relative_target = target_path.is_absolute()
            ? target_path.lexically_relative("/")
            : target_path;
        if(relative_target.empty()){
            return std::unexpected(sandbox_error::invalid_argument);
        }

        const std::filesystem::path full_target_path = rootfs_path / relative_target;
        std::error_code type_ec;
        const bool is_directory = std::filesystem::is_directory(source_path, type_ec);
        if(type_ec){
            return std::unexpected(sandbox_error::from_error_code(type_ec));
        }

        if(is_directory){
            const auto create_directories_exp = file_util::create_directories(full_target_path);
            if(!create_directories_exp){
                return std::unexpected(sandbox_error{create_directories_exp.error()});
            }
            return {};
        }

        const auto create_parent_dirs_exp = file_util::create_directories(
            full_target_path.parent_path()
        );
        if(!create_parent_dirs_exp){
            return std::unexpected(sandbox_error{create_parent_dirs_exp.error()});
        }

        return create_empty_file(full_target_path);
    }

    std::expected<void, sandbox_error> append_mount_if_exists(
        std::vector<std::string>& sandbox_command_args,
        const std::filesystem::path& rootfs_path,
        const std::filesystem::path& source_path,
        const std::filesystem::path& target_path,
        bool writable
    ){
        std::error_code exists_ec;
        const bool exists_value = std::filesystem::exists(source_path, exists_ec);
        if(exists_ec){
            return std::unexpected(sandbox_error::from_error_code(exists_ec));
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

    std::expected<void, sandbox_error> prepare_fixed_mount_args(
        std::vector<std::string>& fixed_mount_args,
        const std::filesystem::path& rootfs_path
    ){
        fixed_mount_args.clear();
        fixed_mount_args.reserve((readonly_mount_paths.size() + readonly_device_paths.size()) * 2);

        for(const auto& ro_mount_text : readonly_mount_paths){
            const std::filesystem::path ro_mount{ro_mount_text};
            const auto append_mount_exp = append_mount_if_exists(
                fixed_mount_args,
                rootfs_path,
                ro_mount,
                ro_mount,
                false
            );
            if(!append_mount_exp){
                return std::unexpected(append_mount_exp.error());
            }
        }

        for(const auto& device_path_text : readonly_device_paths){
            const std::filesystem::path device_path{device_path_text};
            const auto append_mount_exp = append_mount_if_exists(
                fixed_mount_args,
                rootfs_path,
                device_path,
                device_path,
                false
            );
            if(!append_mount_exp){
                return std::unexpected(append_mount_exp.error());
            }
        }

        return {};
    }

    std::expected<std::filesystem::path, sandbox_error> prepare_rootfs_layout(
        const temp_dir& sandbox_dir
    ){
        const std::filesystem::path rootfs_path = sandbox_dir.get_path() / "rootfs";
        const auto create_rootfs_exp = file_util::create_directories(rootfs_path);
        if(!create_rootfs_exp){
            return std::unexpected(sandbox_error{create_rootfs_exp.error()});
        }

        for(const std::filesystem::path& relative_dir : {
                std::filesystem::path{"workspace"},
                std::filesystem::path{"tmp"},
                std::filesystem::path{"proc"},
                std::filesystem::path{"dev"}
            }){
            const auto create_directory_exp = file_util::create_directories(rootfs_path / relative_dir);
            if(!create_directory_exp){
                return std::unexpected(sandbox_error{create_directory_exp.error()});
            }
        }

        return rootfs_path;
    }

    std::expected<std::filesystem::path, sandbox_error> create_seccomp_policy_file(
        const temp_dir& sandbox_dir,
        sandbox_runner::policy_profile policy_profile_value
    ){
        const std::filesystem::path policy_path = sandbox_dir.get_path() / "policy.kafel";
        const auto write_policy_exp = file_util::create_file(
            policy_path,
            make_seccomp_policy_text(policy_profile_value)
        );
        if(!write_policy_exp){
            return std::unexpected(sandbox_error{write_policy_exp.error()});
        }

        return policy_path;
    }

    std::int64_t make_process_limit(const sandbox_runner::run_options& run_options_value){
        if(run_options_value.policy == sandbox_runner::policy_profile::compile){
            return compile_nproc_limit;
        }

        if(run_options_value.mounts == sandbox_runner::mount_profile::java_profile){
            return java_run_nproc_limit;
        }

        return default_run_nproc_limit;
    }

    std::expected<void, sandbox_error> validate_sandbox_artifacts(
        const nsjail_util::sandbox_artifacts& sandbox_artifacts_value
    ){
        std::error_code exists_ec;
        const bool rootfs_exists = std::filesystem::is_directory(
            sandbox_artifacts_value.rootfs_path_,
            exists_ec
        );
        if(exists_ec){
            return std::unexpected(make_invalid_configuration(io_error::from_error_code(exists_ec)));
        }
        if(!rootfs_exists){
            return std::unexpected(make_invalid_configuration("sandbox rootfs not found"));
        }

        for(const std::filesystem::path& relative_dir : {
                std::filesystem::path{"workspace"},
                std::filesystem::path{"tmp"},
                std::filesystem::path{"proc"},
                std::filesystem::path{"dev"}
            }){
            const bool subdir_exists = std::filesystem::is_directory(
                sandbox_artifacts_value.rootfs_path_ / relative_dir,
                exists_ec
            );
            if(exists_ec){
                return std::unexpected(
                    make_invalid_configuration(io_error::from_error_code(exists_ec))
                );
            }
            if(!subdir_exists){
                return std::unexpected(
                    make_invalid_configuration(
                        "sandbox rootfs entry missing: " + relative_dir.string()
                    )
                );
            }
        }

        const bool seccomp_exists = std::filesystem::exists(
            sandbox_artifacts_value.seccomp_policy_path_,
            exists_ec
        );
        if(exists_ec){
            return std::unexpected(make_invalid_configuration(io_error::from_error_code(exists_ec)));
        }
        if(!seccomp_exists){
            return std::unexpected(
                make_invalid_configuration("sandbox seccomp policy file not found")
            );
        }

        return {};
    }

    struct sandbox_artifact_cache{
        std::mutex mutex_;
        std::shared_ptr<const nsjail_util::sandbox_artifacts> compile_artifacts_;
        std::shared_ptr<const nsjail_util::sandbox_artifacts> run_artifacts_;
    };

    sandbox_artifact_cache& get_sandbox_artifact_cache(){
        static sandbox_artifact_cache cache;
        return cache;
    }

    std::shared_ptr<const nsjail_util::sandbox_artifacts>& get_cache_slot(
        sandbox_artifact_cache& cache,
        sandbox_runner::policy_profile policy_profile_value
    ){
        return policy_profile_value == sandbox_runner::policy_profile::compile
            ? cache.compile_artifacts_
            : cache.run_artifacts_;
    }

    std::expected<nsjail_util::sandbox_artifacts, sandbox_error> build_sandbox_artifacts(
        sandbox_runner::policy_profile policy_profile_value
    ){
        auto sandbox_dir_exp = temp_dir::create("/tmp/oj_nsjail_XXXXXX");
        if(!sandbox_dir_exp){
            return std::unexpected(sandbox_error{sandbox_dir_exp.error()});
        }

        const auto rootfs_path_exp = prepare_rootfs_layout(*sandbox_dir_exp);
        if(!rootfs_path_exp){
            return std::unexpected(rootfs_path_exp.error());
        }

        const auto seccomp_policy_path_exp = create_seccomp_policy_file(
            *sandbox_dir_exp,
            policy_profile_value
        );
        if(!seccomp_policy_path_exp){
            return std::unexpected(seccomp_policy_path_exp.error());
        }

        nsjail_util::sandbox_artifacts sandbox_artifacts_value;
        sandbox_artifacts_value.sandbox_dir_ = std::move(*sandbox_dir_exp);
        sandbox_artifacts_value.rootfs_path_ = *rootfs_path_exp;
        sandbox_artifacts_value.seccomp_policy_path_ = *seccomp_policy_path_exp;

        const auto prepare_fixed_mount_args_exp = prepare_fixed_mount_args(
            sandbox_artifacts_value.fixed_mount_args_,
            sandbox_artifacts_value.rootfs_path_
        );
        if(!prepare_fixed_mount_args_exp){
            return std::unexpected(prepare_fixed_mount_args_exp.error());
        }

        return sandbox_artifacts_value;
    }
}

std::expected<std::filesystem::path, sandbox_error> nsjail_util::require_nsjail_path(){
    const auto nsjail_path_exp = env_util::require_env("JUDGE_NSJAIL_PATH");
    if(!nsjail_path_exp){
        return std::unexpected(detail::make_invalid_configuration(nsjail_path_exp.error()));
    }

    const std::filesystem::path nsjail_path = *nsjail_path_exp;
    if(::access(nsjail_path.c_str(), X_OK) < 0){
        return std::unexpected(
            detail::make_invalid_configuration(io_error::from_errno(errno))
        );
    }

    return nsjail_path;
}

std::expected<void, sandbox_error> nsjail_util::validate_workspace_path(
    const std::filesystem::path& workspace_host_path
){
    if(workspace_host_path.empty() || !workspace_host_path.is_absolute()){
        return std::unexpected(sandbox_error::invalid_argument);
    }

    std::error_code exists_ec;
    const bool exists_value = std::filesystem::exists(workspace_host_path, exists_ec);
    if(exists_ec){
        return std::unexpected(sandbox_error::from_error_code(exists_ec));
    }
    if(!exists_value){
        return std::unexpected(
            detail::make_invalid_configuration("sandbox workspace path not found")
        );
    }

    return {};
}

std::expected<void, sandbox_error> nsjail_util::check_user_namespace_support(){
    int error_pipe[2];
    if(::pipe(error_pipe) < 0){
        return std::unexpected(sandbox_error{
            sandbox_error_code::internal,
            "pipe failed"
        });
    }

    unique_fd read_fd(error_pipe[0]);
    unique_fd write_fd(error_pipe[1]);

    const pid_t pid = ::fork();
    if(pid < 0){
        return std::unexpected(sandbox_error{
            sandbox_error_code::internal,
            "fork failed"
        });
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

        return std::unexpected(sandbox_error{
            sandbox_error_code::internal,
            "waitpid failed"
        });
    }

    int error_number = 0;
    const auto read_error_exp = blocking_io::read_all(read_fd.get());
    if(!read_error_exp){
        return std::unexpected(sandbox_error{read_error_exp.error()});
    }
    if(read_error_exp->size() == sizeof(error_number)){
        std::memcpy(&error_number, read_error_exp->data(), sizeof(error_number));
    }

    if(WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 0){
        return {};
    }

    if(error_number != 0){
        const io_error user_namespace_error = io_error::from_errno(error_number);
        if(user_namespace_error.code == io_error_code::unavailable){
            return std::unexpected(sandbox_error{
                sandbox_error_code::unavailable,
                user_namespace_error.message
            });
        }

        return std::unexpected(sandbox_error{
            sandbox_error_code::unsupported,
            user_namespace_error.message
        });
    }

    return std::unexpected(sandbox_error::unsupported);
}

std::expected<std::shared_ptr<const nsjail_util::sandbox_artifacts>, sandbox_error>
nsjail_util::acquire_sandbox_artifacts(
    sandbox_runner::policy_profile policy_profile_value
){
    auto& cache = detail::get_sandbox_artifact_cache();
    std::scoped_lock lock(cache.mutex_);

    auto& cache_slot = detail::get_cache_slot(cache, policy_profile_value);
    if(cache_slot){
        const auto validate_sandbox_artifacts_exp = detail::validate_sandbox_artifacts(*cache_slot);
        if(validate_sandbox_artifacts_exp){
            return cache_slot;
        }

        cache_slot.reset();
    }

    auto build_sandbox_artifacts_exp = detail::build_sandbox_artifacts(policy_profile_value);
    if(!build_sandbox_artifacts_exp){
        return std::unexpected(build_sandbox_artifacts_exp.error());
    }

    cache_slot = std::make_shared<const sandbox_artifacts>(
        std::move(*build_sandbox_artifacts_exp)
    );
    return cache_slot;
}

void nsjail_util::invalidate_all_sandbox_artifacts() noexcept{
    auto& cache = detail::get_sandbox_artifact_cache();
    std::scoped_lock lock(cache.mutex_);
    cache.compile_artifacts_.reset();
    cache.run_artifacts_.reset();
}

std::expected<std::vector<std::string>, sandbox_error> nsjail_util::make_command_args(
    const std::filesystem::path& nsjail_path,
    const sandbox_artifacts& sandbox_artifacts_value,
    const std::vector<std::string>& command_args,
    const sandbox_runner::run_options& run_options_value
){
    const auto cpu_limit_seconds_value =
        std::max<std::int64_t>(1, (run_options_value.time_limit.count() + 999) / 1000);
    const auto wall_time_limit_seconds_value =
        std::max<std::int64_t>(1, (run_options_value.time_limit.count() + 999) / 1000);

    std::vector<std::string> sandbox_command_args;
    sandbox_command_args.reserve(
        48 + sandbox_artifacts_value.fixed_mount_args_.size() + command_args.size()
    );
    sandbox_command_args.push_back(nsjail_path.string());
    sandbox_command_args.push_back("-Me");
    sandbox_command_args.push_back("-Q");
    sandbox_command_args.push_back("--disable_clone_newcgroup");
    if(detail::should_disable_network_namespace()){
        sandbox_command_args.push_back("--disable_clone_newnet");
    }
    sandbox_command_args.push_back("-c");
    sandbox_command_args.push_back(sandbox_artifacts_value.rootfs_path_.string());
    sandbox_command_args.push_back("-D");
    sandbox_command_args.push_back(std::string(detail::sandbox_workspace_path));
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
    sandbox_command_args.push_back(std::to_string(detail::sandbox_nofile_limit));
    sandbox_command_args.push_back("--rlimit_fsize");
    sandbox_command_args.push_back(std::to_string(detail::output_file_limit_mb));
    sandbox_command_args.push_back("--rlimit_core");
    sandbox_command_args.push_back("0");
    sandbox_command_args.push_back("--rlimit_nproc");
    sandbox_command_args.push_back(std::to_string(detail::make_process_limit(run_options_value)));
    sandbox_command_args.push_back("-P");
    sandbox_command_args.push_back(sandbox_artifacts_value.seccomp_policy_path_.string());
    sandbox_command_args.insert(
        sandbox_command_args.end(),
        sandbox_artifacts_value.fixed_mount_args_.begin(),
        sandbox_artifacts_value.fixed_mount_args_.end()
    );

    const auto append_workspace_mount_exp = detail::append_mount_if_exists(
        sandbox_command_args,
        sandbox_artifacts_value.rootfs_path_,
        run_options_value.workspace_host_path,
        detail::sandbox_workspace_path,
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
