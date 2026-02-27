#include "judge_server/core/temp_file.hpp"

#include <cstdlib>
#include <utility>
#include <vector>

temp_file::temp_file(unique_fd fd, std::filesystem::path path) noexcept
    : fd_(std::move(fd)), path_(std::move(path)){}

temp_file::temp_file(temp_file&& other) noexcept
    : fd_(std::move(other.fd_)), path_(std::move(other.path_)){
    other.path_.clear();
}

temp_file& temp_file::operator=(temp_file&& other) noexcept{
    if(this != &other){
        remove_file();
        fd_ = std::move(other.fd_);
        path_ = std::move(other.path_);
        other.path_.clear();
    }
    return *this;
}

temp_file::~temp_file() noexcept{
    remove_file();
}

std::expected<temp_file, error_code> temp_file::create(std::string_view pattern){
    std::vector<char> temp(pattern.begin(), pattern.end());
    temp.push_back('\0');

    int fd = mkstemp(temp.data());
    if(fd < 0){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    return temp_file(unique_fd(fd), std::filesystem::path(temp.data()));
}

void temp_file::close_fd() noexcept{
    fd_.close();
}

int temp_file::get_fd() const noexcept{
    return fd_.get();
}

const std::filesystem::path& temp_file::get_path() const noexcept{
    return path_;
}

temp_file::operator bool() const noexcept{
    return static_cast<bool>(fd_);
}

void temp_file::remove_file() noexcept{
    if(path_.empty()) return;
    std::error_code ignore_ec;
    std::filesystem::remove(path_, ignore_ec);
    path_.clear();
}
