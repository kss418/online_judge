#include "common/file_utility.hpp"

#include <fstream>
#include <string>

std::expected<void, error_code> file_utility::create_file(
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

std::filesystem::path file_utility::make_source_file_path(
    const std::filesystem::path& source_root_path,
    std::int64_t submission_id,
    std::string_view language
){
    std::string_view extension = ".txt";
    if(language == "cpp"){
        extension = ".cpp";
    }
    else if(language == "python"){
        extension = ".py";
    }
    else if(language == "java"){
        extension = ".java";
    }

    return source_root_path / (std::to_string(submission_id) + std::string(extension));
}
