#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace{
    io_error make_invalid_argument_error(const char* message){
        return io_error{io_error_code::invalid_argument, message};
    }

    std::expected<std::filesystem::path, io_error> validate_testcase_base_path(
        const std::filesystem::path& testcase_base_path
    ){
        if(testcase_base_path.empty()){
            return std::unexpected(make_invalid_argument_error("invalid testcase base path"));
        }

        return testcase_base_path;
    }

    std::expected<std::filesystem::path, io_error> make_validated_testcase_path(
        const std::filesystem::path& testcase_base_path,
        std::filesystem::path relative_path
    ){
        const auto validated_testcase_base_path_exp = validate_testcase_base_path(
            testcase_base_path
        );
        if(!validated_testcase_base_path_exp){
            return std::unexpected(validated_testcase_base_path_exp.error());
        }

        return *validated_testcase_base_path_exp / std::move(relative_path);
    }

    std::string format_testcase_file_name(
        std::int32_t order,
        std::string_view extension
    ){
        std::ostringstream file_name_stream;
        file_name_stream << std::setw(3) << std::setfill('0') << order << extension;
        return file_name_stream.str();
    }
}

std::int32_t testcase_snapshot::case_count() const noexcept{
    return testcase_count;
}

std::expected<void, io_error> testcase_snapshot::validate() const{
    if(problem_id <= 0){
        return std::unexpected(make_invalid_argument_error("invalid problem id"));
    }

    if(version <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase version"));
    }

    if(testcase_count < 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase count"));
    }

    const auto validated_directory_path_exp = validate_testcase_base_path(directory_path);
    if(!validated_directory_path_exp){
        return std::unexpected(validated_directory_path_exp.error());
    }

    if(!problem_content_dto::is_valid(limits_value)){
        return std::unexpected(make_invalid_argument_error("invalid testcase limits"));
    }

    return {};
}

std::expected<std::filesystem::path, io_error> testcase_snapshot::input_path(
    std::int32_t order
) const{
    if(order <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase order"));
    }

    return make_validated_testcase_path(
        directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".in"))
    );
}

std::expected<std::filesystem::path, io_error> testcase_snapshot::output_path(
    std::int32_t order
) const{
    if(order <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase order"));
    }

    return make_validated_testcase_path(
        directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".out"))
    );
}
