#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"

#include "judge_core/testcase_snapshot/snapshot_layout.hpp"

namespace{
    io_error make_invalid_argument_error(const char* message){
        return io_error{io_error_code::invalid_argument, message};
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

    if(directory_path.empty()){
        return std::unexpected(make_invalid_argument_error("invalid testcase base path"));
    }

    if(!problem_content_dto::is_valid(limits_value)){
        return std::unexpected(make_invalid_argument_error("invalid testcase limits"));
    }

    return {};
}

std::expected<std::filesystem::path, io_error> testcase_snapshot::input_path(
    std::int32_t order
) const{
    return snapshot_layout::make_input_path(directory_path, order);
}

std::expected<std::filesystem::path, io_error> testcase_snapshot::output_path(
    std::int32_t order
) const{
    return snapshot_layout::make_output_path(directory_path, order);
}
