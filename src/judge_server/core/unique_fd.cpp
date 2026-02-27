#include "judge_server/core/unique_fd.hpp"
#include <unistd.h>

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

int unique_fd::get() const noexcept{ return fd_; }
unique_fd::operator bool() const noexcept{ return fd_ != -1; }
unique_fd::unique_fd(int fd) noexcept : fd_(fd){}
