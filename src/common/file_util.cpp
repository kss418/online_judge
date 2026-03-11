#include "common/file_util.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <system_error>

file_utility& file_utility::instance(){
    static file_utility file_utility_value;
    file_utility_value.initialize_if_needed();
    return file_utility_value;
}

void file_utility::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(source_directory_path_.has_value() && testcase_root_path_.has_value()){
        return;
    }

    const char* source_directory_path = std::getenv("JUDGE_SOURCE_ROOT");
    if(source_directory_path != nullptr && *source_directory_path != '\0'){
        source_directory_path_ = std::filesystem::path(source_directory_path);
    }

    const char* testcase_root_path = std::getenv("TESTCASE_PATH");
    if(testcase_root_path != nullptr && *testcase_root_path != '\0'){
        testcase_root_path_ = std::filesystem::path(testcase_root_path);
    }
}

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

std::expected<std::int32_t, error_code> file_utility::read_int32_file(
    const std::filesystem::path& file_path
){
    const auto file_exists_exp = exists(file_path);
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

std::expected<std::filesystem::path, error_code> file_utility::make_source_directory_path(){
    if(!source_directory_path_.has_value() || source_directory_path_->empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return *source_directory_path_;
}

std::expected<std::filesystem::path, error_code> file_utility::make_source_file_path(
    std::int64_t submission_id,
    std::string_view language
){
    const auto source_root_path_exp = make_source_directory_path();
    if(!source_root_path_exp){
        return std::unexpected(source_root_path_exp.error());
    }

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

    return *source_root_path_exp / (std::to_string(submission_id) + std::string(extension));
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_problem_directory_path(
    std::int64_t problem_id
){
    if(!testcase_root_path_.has_value() || testcase_root_path_->empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return *testcase_root_path_ / std::to_string(problem_id);
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_input_path(
    std::int64_t problem_id,
    std::int32_t order
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / (std::to_string(order) + ".in");
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_output_path(
    std::int64_t problem_id,
    std::int32_t order
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / (std::to_string(order) + ".out");
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_version_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "version";
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_memory_limit_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "memory_limit";
}

std::expected<std::filesystem::path, error_code> file_utility::make_testcase_time_limit_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "time_limit";
}

std::expected<std::int32_t, error_code> file_utility::count_testcase_output(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_iterator(*problem_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
    }

    std::int32_t testcase_count = 0;
    for(std::filesystem::directory_iterator end; directory_iterator != end; ++directory_iterator){
        if(directory_iterator->path().extension() == ".out"){
            ++testcase_count;
        }
    }

    return testcase_count;
}

std::expected<std::int32_t, error_code> file_utility::validate_testcase_output(
    std::int64_t problem_id, std::int32_t testcase_count
){
    if(testcase_count < 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto problem_directory_path_exp = make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    std::vector<std::int32_t> testcase_orders;
    testcase_orders.reserve(static_cast<std::size_t>(testcase_count));

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_iterator(*problem_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
    }

    for(std::filesystem::directory_iterator end; directory_iterator != end; ++directory_iterator){
        const std::filesystem::path entry_path = directory_iterator->path();
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
