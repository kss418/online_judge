#pragma once
#include <boost/system/error_code.hpp>

#include <exception>
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

enum class boost_error{
    operation_aborted,
    timed_out,
    would_block,
    try_again,
    bad_descriptor,
    already_open,
    not_connected,
    connection_refused,
    connection_reset,
    connection_aborted,
    eof,
    end_of_stream,
    unknown_boost_error
};

enum class psql_error{
    protocol_violation,
    in_doubt_error,
    serialization_failure,
    deadlock_detected,
    statement_completion_unknown,
    unique_violation,
    foreign_key_violation,
    not_null_violation,
    check_violation,
    feature_not_supported,
    data_exception,
    sql_error,
    broken_connection,
    unknown_psql_error
};

enum class error_type{
    syscall_type,
    errno_type,
    signal_type,
    limit_type,
    boost_type,
    psql_type
};

struct error_code{
    error_type type_;
    int code_ = 0;

    constexpr bool operator==(const error_code&) const = default;

    static error_code create(syscall_error code);
    static error_code create(errno_error code);
    static error_code create(signal_error code);
    static error_code create(limit_error code);
    static error_code create(boost_error code);
    static error_code create(psql_error code);

    static errno_error map_errno(int code);
    static boost_error map_boost_error(const boost::system::error_code& ec);
    static error_code map_boost_error_code(const boost::system::error_code& ec);
    static psql_error map_psql_error(const std::exception& exception);
    static error_code map_psql_error_code(const std::exception& exception);
    static signal_error map_signal(int signal_number);

    friend constexpr bool operator==(const error_code& left, syscall_error right){
        return
            left.type_ == error_type::syscall_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(syscall_error left, const error_code& right){
        return right == left;
    }

    friend constexpr bool operator==(const error_code& left, errno_error right){
        return
            left.type_ == error_type::errno_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(errno_error left, const error_code& right){
        return right == left;
    }

    friend constexpr bool operator==(const error_code& left, signal_error right){
        return
            left.type_ == error_type::signal_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(signal_error left, const error_code& right){
        return right == left;
    }

    friend constexpr bool operator==(const error_code& left, limit_error right){
        return
            left.type_ == error_type::limit_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(limit_error left, const error_code& right){
        return right == left;
    }

    friend constexpr bool operator==(const error_code& left, boost_error right){
        return
            left.type_ == error_type::boost_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(boost_error left, const error_code& right){
        return right == left;
    }

    friend constexpr bool operator==(const error_code& left, psql_error right){
        return
            left.type_ == error_type::psql_type &&
            left.code_ == static_cast<int>(right);
    }

    friend constexpr bool operator==(psql_error left, const error_code& right){
        return right == left;
    }
};

std::string to_string(error_code ec);
std::string to_string(boost_error ec);
std::string to_string(psql_error ec);
