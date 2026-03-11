#include "common/file_util.hpp"

#include <fstream>

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
    const auto file_exists_exp = file_util::exists(file_path);
    if(!file_exists_exp){
        return std::unexpected(file_exists_exp.error());
    }

    if(!file_exists_exp.value()){
        return std::unexpected(error_code::create(errno_error::file_not_found));
    }

    std::ifstream value_file(file_path);
    if(!value_file.is_open()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    std::int32_t value = 0;
    value_file >> value;
    if(value_file.bad()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    if(!value_file){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    value_file >> std::ws;
    if(!value_file.eof()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return value;
}

std::expected<void, error_code> file_util::create_file(
    const std::filesystem::path& file_path,
    std::string_view file_content
){
    std::ofstream file_stream(file_path, std::ios::out | std::ios::trunc | std::ios::binary);
    if(!file_stream.is_open()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    file_stream.write(file_content.data(), static_cast<std::streamsize>(file_content.size()));
    if(!file_stream.good()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    return {};
}
