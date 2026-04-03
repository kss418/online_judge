#include "error/io_error.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <string_view>
#include <utility>

namespace{
    struct io_error_spec{
        std::string_view default_message;
    };

    struct errno_mapping{
        io_error_code code;
        std::string_view message;
        bool retryable = false;
    };

    constexpr io_error_spec unknown_io_error_spec{
        "unknown io error"
    };

    constexpr std::array<io_error_spec, 6> io_error_specs{{
        {"invalid io argument"},
        {"io permission denied"},
        {"io resource not found"},
        {"io conflict"},
        {"io unavailable"},
        {"io internal error"},
    }};

    const io_error_spec& describe_io_error(io_error_code code){
        const auto index = static_cast<std::size_t>(code);
        if(index >= io_error_specs.size()){
            return unknown_io_error_spec;
        }

        return io_error_specs[index];
    }

    errno_mapping map_errno(int error_number){
        switch(error_number){
            case E2BIG:
                return {io_error_code::invalid_argument, "argument list too long"};
            case EACCES:
                return {io_error_code::permission_denied, "permission denied"};
            case EAGAIN:
                return {
                    io_error_code::unavailable,
                    "resource temporarily unavailable",
                    true
                };
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
                return {io_error_code::unavailable, "operation would block", true};
#endif
            case EBADF:
                return {io_error_code::internal, "invalid file descriptor"};
#ifdef EBUSY
            case EBUSY:
                return {io_error_code::unavailable, "resource busy", true};
#endif
#ifdef EDQUOT
            case EDQUOT:
                return {io_error_code::unavailable, "disk quota exceeded"};
#endif
            case EEXIST:
                return {io_error_code::conflict, "file exists"};
            case EFAULT:
                return {io_error_code::internal, "bad address"};
            case EINTR:
                return {
                    io_error_code::unavailable,
                    "interrupted system call",
                    true
                };
            case EINVAL:
                return {io_error_code::invalid_argument, "invalid argument"};
            case EIO:
                return {io_error_code::internal, "io error"};
            case EISDIR:
                return {io_error_code::invalid_argument, "is directory"};
            case ELOOP:
                return {io_error_code::invalid_argument, "too many symbolic links"};
            case EMFILE:
                return {
                    io_error_code::unavailable,
                    "too many open files in process",
                    true
                };
            case ENFILE:
                return {
                    io_error_code::unavailable,
                    "too many open files in system",
                    true
                };
            case ENAMETOOLONG:
                return {io_error_code::invalid_argument, "filename too long"};
            case ENOENT:
                return {io_error_code::not_found, "file not found"};
            case ENOMEM:
                return {io_error_code::internal, "out of memory"};
            case ENODEV:
                return {io_error_code::not_found, "no such device"};
            case ENXIO:
                return {io_error_code::not_found, "no such device or address"};
            case ENOTDIR:
                return {io_error_code::not_found, "not a directory"};
            case ENOEXEC:
                return {io_error_code::invalid_argument, "exec format error"};
            case ENOSPC:
                return {io_error_code::unavailable, "no space left on device"};
            case EPERM:
                return {io_error_code::permission_denied, "operation not permitted"};
            case EPIPE:
                return {io_error_code::unavailable, "broken pipe"};
            case EROFS:
                return {io_error_code::permission_denied, "read only file system"};
            case ESPIPE:
                return {io_error_code::invalid_argument, "illegal seek"};
            case ETXTBSY:
                return {io_error_code::unavailable, "text file busy", true};
            case EXDEV:
                return {io_error_code::invalid_argument, "cross device link"};
#ifdef EOPNOTSUPP
            case EOPNOTSUPP:
                return {io_error_code::invalid_argument, "operation not supported"};
#endif
        }

        return {io_error_code::internal, "unknown io error"};
    }
}

io_error::io_error(
    io_error_code code_value,
    std::string message_value,
    bool retryable_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_io_error(code_value).default_message}
            : std::move(message_value)
    ),
    retryable(retryable_value){}

bool io_error::operator==(const io_error& other) const{
    return code == other.code;
}

bool io_error::is_retryable() const{
    return retryable;
}

io_error io_error::from_errno(int error_number){
    const errno_mapping mapping = map_errno(error_number);
    return io_error{
        mapping.code,
        std::string{mapping.message},
        mapping.retryable
    };
}

io_error io_error::from_error_code(const std::error_code& error){
    return from_errno(error.value());
}

const io_error io_error::invalid_argument{
    io_error_code::invalid_argument
};
const io_error io_error::permission_denied{
    io_error_code::permission_denied
};
const io_error io_error::not_found{
    io_error_code::not_found
};
const io_error io_error::conflict{
    io_error_code::conflict
};
const io_error io_error::unavailable{
    io_error_code::unavailable
};
const io_error io_error::internal{
    io_error_code::internal
};

std::string to_string(io_error_code code){
    return std::string{describe_io_error(code).default_message};
}

std::string to_string(const io_error& error){
    return error.message;
}
