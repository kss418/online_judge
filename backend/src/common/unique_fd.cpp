#include "common/unique_fd.hpp"

#include <cerrno>
#include <unistd.h>

static bool should_suppress_close_errno(int error_number){
    return error_number == EINTR;
}

unique_fd::unique_fd(unique_fd&& other) noexcept{
    this->fd_ = other.fd_;
    other.fd_ = -1;
}

unique_fd& unique_fd::operator=(unique_fd&& other) noexcept{
    if(this != &other){
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

unique_fd::~unique_fd() noexcept{ close(); }

void unique_fd::close(int new_fd) noexcept{
    if(fd_ != -1) ::close(fd_);
    fd_ = new_fd;
}

std::expected<void, error_code> unique_fd::close_checked() noexcept{
    if(fd_ == -1){
        return {};
    }

    const int closed_fd = fd_;
    fd_ = -1;
    if(::close(closed_fd) < 0){
        const int close_errno = errno;
        if(should_suppress_close_errno(close_errno)){
            return {};
        }

        return std::unexpected(error_code::create(error_code::map_errno(close_errno)));
    }

    return {};
}

int unique_fd::get() const noexcept{ return fd_; }
unique_fd::operator bool() const noexcept{ return fd_ != -1; }
unique_fd::unique_fd(int fd) noexcept : fd_(fd){}
