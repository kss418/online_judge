#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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
    if(testcase_count < 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase count"));
    }

    const auto validated_directory_path_exp = validate_testcase_base_path(directory_path);
    if(!validated_directory_path_exp){
        return std::unexpected(validated_directory_path_exp.error());
    }

    std::vector<std::int32_t> testcase_orders;
    testcase_orders.reserve(static_cast<std::size_t>(testcase_count));

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_it(*validated_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(io_error::from_error_code(iterator_ec));
    }

    for(std::filesystem::directory_iterator end_it; directory_it != end_it; ++directory_it){
        const std::filesystem::path entry_path = directory_it->path();
        if(entry_path.extension() != ".out"){
            continue;
        }

        const std::string stem = entry_path.stem().string();
        std::int32_t order = 0;
        const auto [parse_end, parse_ec] = std::from_chars(
            stem.data(),
            stem.data() + stem.size(),
            order
        );
        if(
            parse_ec != std::errc{} ||
            parse_end != stem.data() + stem.size() ||
            order <= 0 ||
            order > testcase_count
        ){
            return std::unexpected(
                make_invalid_argument_error("invalid testcase output layout")
            );
        }

        testcase_orders.push_back(order);
    }

    std::sort(testcase_orders.begin(), testcase_orders.end());
    for(std::int32_t index = 0; index < testcase_count; ++index){
        if(
            static_cast<std::size_t>(index) >= testcase_orders.size() ||
            testcase_orders[static_cast<std::size_t>(index)] != index + 1
        ){
            return std::unexpected(
                make_invalid_argument_error("invalid testcase output layout")
            );
        }
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

std::expected<std::filesystem::path, io_error> testcase_snapshot::memory_limit_file_path(
) const{
    return make_validated_testcase_path(
        directory_path,
        std::filesystem::path("memory_limit")
    );
}

std::expected<std::filesystem::path, io_error> testcase_snapshot::time_limit_file_path(
) const{
    return make_validated_testcase_path(
        directory_path,
        std::filesystem::path("time_limit")
    );
}
