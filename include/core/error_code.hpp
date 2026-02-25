#pragma once
#include <string>
#include <expected>

enum syscall_error{
    pipe_failed,
    fork_failed,
    waitpid_failed
};

enum judge_error{
    compile_error,
    runtime_error,
    time_limit_exceed,
    memeory_limit_exceed
};

enum errno_error{
    io_error,
    is_directory,
    bad_address,
    invalid_argument,
    invalid_file_descriptor,
    unknown_error
};

enum class error_type{
    cpp_runner_type,
    judge_type,
    errno_type
};

struct error_code{
    error_type type_;
    int code_ = 0;

    static error_code create(syscall_error code);
    static error_code create(judge_error code);
    static error_code create(errno_error code);

    static errno_error map_errno(int code);
};

std::string to_string(error_code ec);
