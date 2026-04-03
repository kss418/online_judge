#include "common/temp_dir.hpp"

#include <cerrno>
#include <cstdlib>
#include <utility>
#include <vector>

temp_dir::temp_dir(std::filesystem::path path) noexcept
    : path_(std::move(path)){}

temp_dir::temp_dir(temp_dir&& other) noexcept
    : path_(std::move(other.path_)){
    other.path_.clear();
}

temp_dir& temp_dir::operator=(temp_dir&& other) noexcept{
    if(this != &other){
        remove_directory();
        path_ = std::move(other.path_);
        other.path_.clear();
    }
    return *this;
}

temp_dir::~temp_dir() noexcept{
    remove_directory();
}

std::expected<temp_dir, io_error> temp_dir::create(std::string_view pattern){
    std::vector<char> temp(pattern.begin(), pattern.end());
    temp.push_back('\0');

    char* created_path = mkdtemp(temp.data());
    if(created_path == nullptr){
        return std::unexpected(io_error::from_errno(errno));
    }

    return temp_dir(std::filesystem::path(created_path));
}

const std::filesystem::path& temp_dir::get_path() const noexcept{
    return path_;
}

temp_dir::operator bool() const noexcept{
    return !path_.empty();
}

void temp_dir::remove_directory() noexcept{
    if(path_.empty()){
        return;
    }

    std::error_code remove_ec;
    std::filesystem::remove_all(path_, remove_ec);
    path_.clear();
}
