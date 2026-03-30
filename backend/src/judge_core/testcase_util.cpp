#include "judge_core/testcase_util.hpp"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace{
    std::expected<std::filesystem::path, error_code> validate_testcase_base_path(
        const std::filesystem::path& testcase_base_path
    ){
        if(testcase_base_path.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return testcase_base_path;
    }

    std::expected<std::filesystem::path, error_code> make_validated_testcase_path(
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

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_problem_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return make_validated_testcase_path(
        testcase_root_path,
        std::filesystem::path(std::to_string(problem_id))
    );
}

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_version_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id,
    std::int32_t version
){
    if(version <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto problem_directory_path_exp = make_testcase_problem_directory_path(
        testcase_root_path,
        problem_id
    );
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return make_validated_testcase_path(
        *problem_directory_path_exp,
        std::filesystem::path(std::to_string(version))
    );
}

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_input_path(
    const std::filesystem::path& testcase_directory_path,
    std::int32_t order
){
    if(order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".in"))
    );
}

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_output_path(
    const std::filesystem::path& testcase_directory_path,
    std::int32_t order
){
    if(order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".out"))
    );
}

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_memory_limit_file_path(
    const std::filesystem::path& testcase_directory_path
){
    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path("memory_limit")
    );
}

std::expected<std::filesystem::path, error_code> testcase_util::make_testcase_time_limit_file_path(
    const std::filesystem::path& testcase_directory_path
){
    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path("time_limit")
    );
}

std::expected<std::int32_t, error_code> testcase_util::count_testcase_output(
    const std::filesystem::path& testcase_directory_path
){
    const auto validated_directory_path_exp = validate_testcase_base_path(testcase_directory_path);
    if(!validated_directory_path_exp){
        return std::unexpected(validated_directory_path_exp.error());
    }

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_it(*validated_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
    }

    std::int32_t testcase_count = 0;
    for(std::filesystem::directory_iterator end_it; directory_it != end_it; ++directory_it){
        if(directory_it->path().extension() == ".out"){
            ++testcase_count;
        }
    }

    return testcase_count;
}

std::expected<std::int32_t, error_code> testcase_util::validate_testcase_output(
    const std::filesystem::path& testcase_directory_path,
    std::int32_t testcase_count
){
    if(testcase_count < 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto validated_directory_path_exp = validate_testcase_base_path(testcase_directory_path);
    if(!validated_directory_path_exp){
        return std::unexpected(validated_directory_path_exp.error());
    }

    std::vector<std::int32_t> testcase_orders;
    testcase_orders.reserve(static_cast<std::size_t>(testcase_count));

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_it(*validated_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
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
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        testcase_orders.push_back(order);
    }

    std::sort(testcase_orders.begin(), testcase_orders.end());
    for(std::int32_t index = 0; index < testcase_count; ++index){
        if(
            static_cast<std::size_t>(index) >= testcase_orders.size() ||
            testcase_orders[static_cast<std::size_t>(index)] != index + 1
        ){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }
    }

    return testcase_count;
}
