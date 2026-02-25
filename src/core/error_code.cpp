#include "core/error_code.hpp"
#include <cerrno>

std::string to_string(syscall_error ec){
    switch(ec){
        case pipe_failed:
            return "pipe failed";
        case fork_failed:
            return "fork failed";
        case waitpid_failed:
            return "waitpid failed";
    }
    return "unknown syscall error";
}

std::string to_string(judge_error ec){
    switch(ec){
        case compile_error:
            return "compile error";
        case runtime_error:
            return "runtime error";
        case time_limit_exceed:
            return "time limit exceed";
        case memeory_limit_exceed:
            return "memory limit exceed";
    }
    return "unknown judge error";
}

std::string to_string(errno_error ec){
    switch(ec){
        case io_error:
            return "io error";
        case is_directory:
            return "is directory";
        case bad_address:
            return "bad address";
        case invalid_argument:
            return "invalid argument";
        case invalid_file_descriptor:
            return "invalid file descriptor";
    }
    return "unknown errno error";
}

std::string to_string(error_code ec){
    if(ec.type_ == error_type::cpp_runner_type) return to_string(static_cast<syscall_error>(ec.code_));
    else if(ec.type_ == error_type::judge_type) return to_string(static_cast<judge_error>(ec.code_));
    else if(ec.type_ == error_type::errno_type) return to_string(static_cast<errno_error>(ec.code_));
    return "unknown error code";
}

error_code error_code::create(syscall_error code){
    return error_code(error_type::cpp_runner_type, code);
}

error_code error_code::create(judge_error code){
    return error_code(error_type::judge_type, code);
}

error_code error_code::create(errno_error code){
    return error_code(error_type::errno_type, code);
}

errno_error error_code::map_errno(int code){
    switch (code) {
        case EBADF: 
            return errno_error::invalid_file_descriptor;
        case EFAULT: 
            return errno_error::bad_address;
        case EISDIR: 
            return errno_error::is_directory;
        case EINVAL: 
            return errno_error::invalid_argument;
        case EIO: 
            return errno_error::io_error;
    }
    return errno_error::unknown_error;
}
