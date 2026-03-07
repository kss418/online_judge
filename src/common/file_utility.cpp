#include "common/file_utility.hpp"

#include <fstream>
#include <string>
#include <system_error>

std::expected<bool, error_code> file_utility::exists(const std::filesystem::path& file_path){
    std::error_code exists_ec;
    const bool exists_value = std::filesystem::exists(file_path, exists_ec);
    if(exists_ec){
        return std::unexpected(error_code::create(error_code::map_errno(exists_ec.value())));
    }

    return exists_value;
}

std::expected<void, error_code> file_utility::create_directories(
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

std::expected<void, error_code> file_utility::remove_file(const std::filesystem::path& file_path){
    std::error_code remove_ec;
    std::filesystem::remove(file_path, remove_ec);
    if(remove_ec){
        return std::unexpected(error_code::create(error_code::map_errno(remove_ec.value())));
    }

    return {};
}

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

std::filesystem::path file_utility::make_testcase_problem_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id
){
    return testcase_root_path / std::to_string(problem_id);
}

std::filesystem::path file_utility::make_testcase_input_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id,
    std::int32_t order
){
    return make_testcase_problem_directory_path(testcase_root_path, problem_id) /
        (std::to_string(order) + ".in");
}

std::filesystem::path file_utility::make_testcase_output_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id,
    std::int32_t order
){
    return make_testcase_problem_directory_path(testcase_root_path, problem_id) /
        (std::to_string(order) + ".out");
}

std::filesystem::path file_utility::make_testcase_version_file_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id
){
    return make_testcase_problem_directory_path(testcase_root_path, problem_id) / "version";
}
