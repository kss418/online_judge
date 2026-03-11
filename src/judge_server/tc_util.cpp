#include "judge_server/tc_util.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <system_error>
#include <vector>

tc_util& tc_util::instance(){
    static tc_util tc_util_value;
    tc_util_value.initialize_if_needed();
    return tc_util_value;
}

void tc_util::initialize_if_needed(){
    std::scoped_lock lock(initialize_mutex_);
    if(tc_root_path_.has_value()){
        return;
    }

    const char* tc_root_path = std::getenv("TESTCASE_PATH");
    if(tc_root_path != nullptr && *tc_root_path != '\0'){
        tc_root_path_ = std::filesystem::path(tc_root_path);
    }
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_problem_directory_path(
    std::int64_t problem_id
){
    if(!tc_root_path_.has_value() || tc_root_path_->empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return *tc_root_path_ / std::to_string(problem_id);
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_input_path(
    std::int64_t problem_id,
    std::int32_t order
){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / (std::to_string(order) + ".in");
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_output_path(
    std::int64_t problem_id,
    std::int32_t order
){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / (std::to_string(order) + ".out");
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_version_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "version";
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_memory_limit_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "memory_limit";
}

std::expected<std::filesystem::path, error_code> tc_util::make_tc_time_limit_file_path(
    std::int64_t problem_id
){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    return *problem_directory_path_exp / "time_limit";
}

std::expected<std::int32_t, error_code> tc_util::count_tc_output(std::int64_t problem_id){
    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_iterator(*problem_directory_path_exp, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
    }

    std::int32_t tc_count = 0;
    for(std::filesystem::directory_iterator end; directory_iterator != end; ++directory_iterator){
        if(directory_iterator->path().extension() == ".out"){
            ++tc_count;
        }
    }

    return tc_count;
}

std::expected<std::int32_t, error_code> tc_util::validate_tc_output(
    std::int64_t problem_id,
    std::int32_t tc_count
){
    if(tc_count < 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto problem_directory_path_exp = make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }

    std::vector<std::int32_t> tc_orders;
    tc_orders.reserve(static_cast<std::size_t>(tc_count));

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
            order > tc_count
        ){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        tc_orders.push_back(order);
    }

    std::sort(tc_orders.begin(), tc_orders.end());
    for(std::int32_t index = 0; index < tc_count; ++index){
        if(
            static_cast<std::size_t>(index) >= tc_orders.size() ||
            tc_orders[static_cast<std::size_t>(index)] != index + 1
        ){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }
    }

    return tc_count;
}
