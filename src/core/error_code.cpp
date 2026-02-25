#include "core/error_code.hpp"

#include <cerrno>
#include <csignal>

std::string to_string(syscall_error ec){
    switch(ec){
        case syscall_error::pipe_failed:
            return "pipe failed";
        case syscall_error::fork_failed:
            return "fork failed";
        case syscall_error::waitpid_failed:
            return "waitpid failed";
    }
    return "unknown syscall error";
}

std::string to_string(errno_error ec){
    switch(ec){
        case errno_error::argument_list_too_long:
            return "argument list too long";
        case errno_error::permission_denied:
            return "permission denied";
        case errno_error::resource_temporarily_unavailable:
            return "resource temporarily unavailable";
        case errno_error::operation_would_block:
            return "operation would block";
        case errno_error::invalid_file_descriptor:
            return "invalid file descriptor";
        case errno_error::busy_resource:
            return "resource busy";
        case errno_error::child_process_not_found:
            return "no child process";
        case errno_error::quota_exceeded:
            return "disk quota exceeded";
        case errno_error::file_exists:
            return "file exists";
        case errno_error::bad_address:
            return "bad address";
        case errno_error::interrupted_system_call:
            return "interrupted system call";
        case errno_error::invalid_argument:
            return "invalid argument";
        case errno_error::io_error:
            return "io error";
        case errno_error::is_directory:
            return "is directory";
        case errno_error::too_many_symbolic_links:
            return "too many symbolic links";
        case errno_error::too_many_open_files_process:
            return "too many open files in process";
        case errno_error::too_many_open_files_system:
            return "too many open files in system";
        case errno_error::filename_too_long:
            return "filename too long";
        case errno_error::file_not_found:
            return "file not found";
        case errno_error::out_of_memory:
            return "out of memory";
        case errno_error::no_such_device:
            return "no such device";
        case errno_error::no_such_device_or_address:
            return "no such device or address";
        case errno_error::not_a_directory:
            return "not a directory";
        case errno_error::exec_format_error:
            return "exec format error";
        case errno_error::no_space_left_on_device:
            return "no space left on device";
        case errno_error::operation_not_permitted:
            return "operation not permitted";
        case errno_error::broken_pipe:
            return "broken pipe";
        case errno_error::read_only_file_system:
            return "read only file system";
        case errno_error::illegal_seek:
            return "illegal seek";
        case errno_error::text_file_busy:
            return "text file busy";
        case errno_error::cross_device_link:
            return "cross device link";
        case errno_error::operation_not_supported:
            return "operation not supported";
        case errno_error::unknown_error:
            return "unknown errno error";
    }
    return "unknown errno error";
}

std::string to_string(signal_error ec){
    switch(ec){
        case signal_error::aborted:
            return "aborted by signal";
        case signal_error::bus_error:
            return "bus error";
        case signal_error::floating_point_exception:
            return "floating point exception";
        case signal_error::illegal_instruction:
            return "illegal instruction";
        case signal_error::interrupted:
            return "interrupted by signal";
        case signal_error::killed:
            return "killed by signal";
        case signal_error::broken_pipe:
            return "broken pipe";
        case signal_error::quit:
            return "quit by signal";
        case signal_error::segmentation_fault:
            return "segmentation fault";
        case signal_error::terminated:
            return "terminated by signal";
        case signal_error::cpu_time_limit_exceeded:
            return "cpu time limit exceeded";
        case signal_error::file_size_limit_exceeded:
            return "file size limit exceeded";
        case signal_error::unknown_signal:
            return "unknown signal";
    }
    return "unknown signal";
}

std::string to_string(error_code ec){
    if(ec.type_ == error_type::syscall_type) return to_string(static_cast<syscall_error>(ec.code_));
    else if(ec.type_ == error_type::errno_type) return to_string(static_cast<errno_error>(ec.code_));
    else if(ec.type_ == error_type::signal_type) return to_string(static_cast<signal_error>(ec.code_));
    return "unknown error code";
}

error_code error_code::create(syscall_error code){
    return error_code{error_type::syscall_type, static_cast<int>(code)};
}

error_code error_code::create(errno_error code){
    return error_code{error_type::errno_type, static_cast<int>(code)};
}

error_code error_code::create(signal_error code){
    return error_code{error_type::signal_type, static_cast<int>(code)};
}

errno_error error_code::map_errno(int code){
    switch(code){
        case E2BIG:
            return errno_error::argument_list_too_long;
        case EACCES:
            return errno_error::permission_denied;
        case EAGAIN:
            return errno_error::resource_temporarily_unavailable;
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
            return errno_error::operation_would_block;
#endif
        case EBADF:
            return errno_error::invalid_file_descriptor;
#ifdef EBUSY
        case EBUSY:
            return errno_error::busy_resource;
#endif
        case ECHILD:
            return errno_error::child_process_not_found;
#ifdef EDQUOT
        case EDQUOT:
            return errno_error::quota_exceeded;
#endif
        case EEXIST:
            return errno_error::file_exists;
        case EFAULT:
            return errno_error::bad_address;
        case EINTR:
            return errno_error::interrupted_system_call;
        case EINVAL:
            return errno_error::invalid_argument;
        case EIO:
            return errno_error::io_error;
        case EISDIR:
            return errno_error::is_directory;
        case ELOOP:
            return errno_error::too_many_symbolic_links;
        case EMFILE:
            return errno_error::too_many_open_files_process;
        case ENFILE:
            return errno_error::too_many_open_files_system;
        case ENAMETOOLONG:
            return errno_error::filename_too_long;
        case ENOENT:
            return errno_error::file_not_found;
        case ENOMEM:
            return errno_error::out_of_memory;
        case ENODEV:
            return errno_error::no_such_device;
        case ENXIO:
            return errno_error::no_such_device_or_address;
        case ENOTDIR:
            return errno_error::not_a_directory;
        case ENOEXEC:
            return errno_error::exec_format_error;
        case ENOSPC:
            return errno_error::no_space_left_on_device;
        case EPERM:
            return errno_error::operation_not_permitted;
        case EPIPE:
            return errno_error::broken_pipe;
        case EROFS:
            return errno_error::read_only_file_system;
        case ESPIPE:
            return errno_error::illegal_seek;
#ifdef ETXTBSY
        case ETXTBSY:
            return errno_error::text_file_busy;
#endif
        case EXDEV:
            return errno_error::cross_device_link;
#if defined(EOPNOTSUPP)
        case EOPNOTSUPP:
            return errno_error::operation_not_supported;
#endif
#if defined(ENOTSUP) && (!defined(EOPNOTSUPP) || ENOTSUP != EOPNOTSUPP)
        case ENOTSUP:
            return errno_error::operation_not_supported;
#endif
    }
    return errno_error::unknown_error;
}

signal_error error_code::map_signal(int signal_number){
    switch(signal_number){
        case SIGABRT:
            return signal_error::aborted;
#ifdef SIGBUS
        case SIGBUS:
            return signal_error::bus_error;
#endif
        case SIGFPE:
            return signal_error::floating_point_exception;
        case SIGILL:
            return signal_error::illegal_instruction;
        case SIGINT:
            return signal_error::interrupted;
        case SIGKILL:
            return signal_error::killed;
        case SIGPIPE:
            return signal_error::broken_pipe;
        case SIGQUIT:
            return signal_error::quit;
        case SIGSEGV:
            return signal_error::segmentation_fault;
        case SIGTERM:
            return signal_error::terminated;
#ifdef SIGXCPU
        case SIGXCPU:
            return signal_error::cpu_time_limit_exceeded;
#endif
#ifdef SIGXFSZ
        case SIGXFSZ:
            return signal_error::file_size_limit_exceeded;
#endif
    }
    return signal_error::unknown_signal;
}
