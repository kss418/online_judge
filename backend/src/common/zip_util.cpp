#include "common/zip_util.hpp"

#include "common/blocking_io.hpp"
#include "common/unique_fd.hpp"
#include "error/io_error_bridge.hpp"

#include <cerrno>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>

namespace zip_util_internal{
    std::string trim_line_endings(std::string_view value){
        std::string trimmed(value);
        while(!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r')){
            trimmed.pop_back();
        }
        return trimmed;
    }

    std::expected<std::string, error_code> run_unzip_command_capture_output(
        const std::vector<std::string>& command_args
    ){
        if(command_args.empty() || command_args[0].empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        int output_pipe[2];
        if(pipe(output_pipe) < 0){
            return std::unexpected(error_code::create(syscall_error::pipe_failed));
        }

        unique_fd read_fd(output_pipe[0]);
        unique_fd write_fd(output_pipe[1]);
        const pid_t child_pid = fork();
        if(child_pid < 0){
            return std::unexpected(error_code::create(syscall_error::fork_failed));
        }

        if(child_pid == 0){
            if(dup2(write_fd.get(), STDOUT_FILENO) < 0){
                _exit(127);
            }
            if(dup2(write_fd.get(), STDERR_FILENO) < 0){
                _exit(127);
            }

            read_fd.close();
            write_fd.close();

            std::vector<char*> argv;
            argv.reserve(command_args.size() + 1);
            for(const auto& command_arg : command_args){
                argv.push_back(const_cast<char*>(command_arg.c_str()));
            }
            argv.push_back(nullptr);

            execvp(command_args[0].c_str(), argv.data());
            _exit(127);
        }

        write_fd.close();

        const auto output_exp = blocking_io::read_all(read_fd.get());
        const auto close_read_fd_exp = read_fd.close_checked();
        if(!output_exp){
            return std::unexpected(io_error_bridge::to_error_code(output_exp.error()));
        }
        if(!close_read_fd_exp){
            return std::unexpected(io_error_bridge::to_error_code(close_read_fd_exp.error()));
        }

        int wait_status = 0;
        while(true){
            if(waitpid(child_pid, &wait_status, 0) == child_pid){
                break;
            }

            if(errno == EINTR){
                continue;
            }

            return std::unexpected(error_code::create(syscall_error::waitpid_failed));
        }

        if(WIFSIGNALED(wait_status)){
            return std::unexpected(error_code::create(error_code::map_signal(WTERMSIG(wait_status))));
        }
        if(!WIFEXITED(wait_status)){
            return std::unexpected(error_code::create(syscall_error::waitpid_failed));
        }

        const int exit_code = WEXITSTATUS(wait_status);
        if(exit_code == 127){
            return std::unexpected(error_code::create(errno_error::file_not_found));
        }
        if(exit_code != 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return *output_exp;
    }
}

std::expected<std::vector<std::string>, error_code> zip_util::list_entry_names(
    const std::filesystem::path& archive_path
){
    const auto archive_listing_exp = zip_util_internal::run_unzip_command_capture_output({
        "unzip",
        "-Z1",
        archive_path.string()
    });
    if(!archive_listing_exp){
        return std::unexpected(archive_listing_exp.error());
    }

    std::vector<std::string> entry_names;
    std::string_view archive_listing = *archive_listing_exp;
    std::size_t line_begin = 0;
    while(line_begin <= archive_listing.size()){
        const std::size_t line_end = archive_listing.find('\n', line_begin);
        const std::string_view raw_line = line_end == std::string_view::npos
            ? archive_listing.substr(line_begin)
            : archive_listing.substr(line_begin, line_end - line_begin);
        const std::string entry_name = zip_util_internal::trim_line_endings(raw_line);
        if(!entry_name.empty()){
            entry_names.push_back(entry_name);
        }

        if(line_end == std::string_view::npos){
            break;
        }
        line_begin = line_end + 1;
    }

    return entry_names;
}

std::expected<void, error_code> zip_util::extract_to_directory(
    const std::filesystem::path& archive_path,
    const std::filesystem::path& output_directory_path
){
    const auto extract_exp = zip_util_internal::run_unzip_command_capture_output({
        "unzip",
        "-qq",
        archive_path.string(),
        "-d",
        output_directory_path.string()
    });
    if(!extract_exp){
        return std::unexpected(extract_exp.error());
    }

    return {};
}
