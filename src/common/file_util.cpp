#include "common/file_util.hpp"
#include "common/temp_file.hpp"
#include "common/unique_fd.hpp"

#include <charconv>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

static std::expected<std::string, error_code> read_file_content(
    const std::filesystem::path& file_path
){
    unique_fd file_descriptor(::open(file_path.c_str(), O_RDONLY));
    if(!file_descriptor){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    std::string file_content;
    char buffer[4096];
    while(true){
        const ssize_t read_count = ::read(file_descriptor.get(), buffer, sizeof(buffer));
        if(read_count == 0){
            break;
        }

        if(read_count < 0){
            return std::unexpected(error_code::create(error_code::map_errno(errno)));
        }

        file_content.append(buffer, static_cast<std::size_t>(read_count));
    }

    const auto close_fd_exp = file_descriptor.close_checked();
    if(!close_fd_exp){
        return std::unexpected(close_fd_exp.error());
    }

    return file_content;
}

static std::expected<void, error_code> write_all(
    int file_descriptor,
    std::string_view file_content
){
    std::size_t written_size = 0;
    while(written_size < file_content.size()){
        const ssize_t written_count = ::write(
            file_descriptor,
            file_content.data() + written_size,
            file_content.size() - written_size
        );
        if(written_count < 0){
            return std::unexpected(error_code::create(error_code::map_errno(errno)));
        }

        written_size += static_cast<std::size_t>(written_count);
    }

    return {};
}

std::expected<bool, error_code> file_util::exists(const std::filesystem::path& file_path){
    std::error_code exists_ec;
    const bool exists_value = std::filesystem::exists(file_path, exists_ec);
    if(exists_ec){
        return std::unexpected(error_code::create(error_code::map_errno(exists_ec.value())));
    }

    return exists_value;
}

std::expected<void, error_code> file_util::create_directories(
    const std::filesystem::path& directory_path
){
    std::error_code create_directories_ec;
    std::filesystem::create_directories(directory_path, create_directories_ec);
    if(create_directories_ec){
        return std::unexpected(
            error_code::create(error_code::map_errno(create_directories_ec.value()))
        );
    }

    return {};
}

std::expected<void, error_code> file_util::remove_file(const std::filesystem::path& file_path){
    std::error_code remove_ec;
    std::filesystem::remove(file_path, remove_ec);
    if(remove_ec){
        return std::unexpected(error_code::create(error_code::map_errno(remove_ec.value())));
    }

    return {};
}

std::expected<std::int32_t, error_code> file_util::read_int32_file(
    const std::filesystem::path& file_path
){
    const auto file_content_exp = read_file_content(file_path);
    if(!file_content_exp){
        return std::unexpected(file_content_exp.error());
    }

    const std::string& file_content = *file_content_exp;
    const char* file_begin = file_content.data();
    const char* file_end = file_begin + file_content.size();

    while(file_begin != file_end && std::isspace(static_cast<unsigned char>(*file_begin))){
        ++file_begin;
    }

    if(file_begin == file_end){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::int32_t value = 0;
    const auto [parse_end, parse_ec] = std::from_chars(file_begin, file_end, value);
    if(parse_ec != std::errc{}){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const char* trailing_begin = parse_end;
    while(trailing_begin != file_end && std::isspace(static_cast<unsigned char>(*trailing_begin))){
        ++trailing_begin;
    }

    if(trailing_begin != file_end){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return value;
}

std::expected<void, error_code> file_util::create_file(
    const std::filesystem::path& file_path,
    std::string_view file_content
){
    std::string temp_file_pattern = file_path.string();
    temp_file_pattern += ".tmp.XXXXXX";

    auto temp_file_exp = temp_file::create(temp_file_pattern);
    if(!temp_file_exp){
        return std::unexpected(temp_file_exp.error());
    }

    temp_file temporary_file = std::move(*temp_file_exp);
    const auto write_exp = write_all(temporary_file.get_fd(), file_content);
    if(!write_exp){
        return std::unexpected(write_exp.error());
    }

    if(::fsync(temporary_file.get_fd()) < 0){
        const int fsync_errno = errno;
        return std::unexpected(error_code::create(error_code::map_errno(fsync_errno)));
    }

    const auto close_fd_exp = temporary_file.close_fd_checked();
    if(!close_fd_exp){
        return std::unexpected(close_fd_exp.error());
    }

    std::error_code rename_ec;
    std::filesystem::rename(temporary_file.get_path(), file_path, rename_ec);
    if(rename_ec){
        return std::unexpected(error_code::create(error_code::map_errno(rename_ec.value())));
    }

    return {};
}
