#pragma once
#include <expected>
#include <string>

enum class syscall_error{
    pipe_failed,
    fork_failed,
    waitpid_failed
};

enum class errno_error{
    argument_list_too_long,
    permission_denied,
    resource_temporarily_unavailable,
    operation_would_block,
    invalid_file_descriptor,
    busy_resource,
    child_process_not_found,
    quota_exceeded,
    file_exists,
    bad_address,
    interrupted_system_call,
    invalid_argument,
    io_error,
    is_directory,
    too_many_symbolic_links,
    too_many_open_files_process,
    too_many_open_files_system,
    filename_too_long,
    file_not_found,
    out_of_memory,
    no_such_device,
    no_such_device_or_address,
    not_a_directory,
    exec_format_error,
    no_space_left_on_device,
    operation_not_permitted,
    broken_pipe,
    read_only_file_system,
    illegal_seek,
    text_file_busy,
    cross_device_link,
    operation_not_supported,
    unknown_error
};

enum class signal_error{
    aborted,
    bus_error,
    floating_point_exception,
    illegal_instruction,
    interrupted,
    killed,
    broken_pipe,
    quit,
    segmentation_fault,
    terminated,
    cpu_time_limit_exceeded,
    file_size_limit_exceeded,
    unknown_signal
};

enum class limit_error{
    invalid_time_limit,
    invalid_memory_limit
};

enum class error_type{
    syscall_type,
    errno_type,
    signal_type,
    limit_type
};

struct error_code{
    error_type type_;
    int code_ = 0;

    static error_code create(syscall_error code);
    static error_code create(errno_error code);
    static error_code create(signal_error code);
    static error_code create(limit_error code);

    static errno_error map_errno(int code);
    static signal_error map_signal(int signal_number);
};

std::string to_string(error_code ec);
