#include "judge_core/testcase_snapshot/snapshot_layout.hpp"

#include "common/file_util.hpp"

#include <charconv>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
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

    std::expected<void, judge_error> ensure_path_exists(
        const std::filesystem::path& path,
        std::string_view message
    ){
        const auto exists_exp = file_util::exists(path);
        if(!exists_exp){
            return std::unexpected(exists_exp.error());
        }

        if(!exists_exp.value()){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    std::string(message)
                }
            );
        }

        return {};
    }

    std::expected<void, judge_error> verify_testcase_output_layout(
        const std::filesystem::path& testcase_directory_path,
        std::int32_t expected_testcase_count
    ){
        const auto validated_directory_path_exp = validate_testcase_base_path(
            testcase_directory_path
        );
        if(!validated_directory_path_exp){
            return std::unexpected(validated_directory_path_exp.error());
        }

        if(expected_testcase_count < 0){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "invalid testcase count"
                }
            );
        }

        std::error_code iterator_ec;
        std::filesystem::directory_iterator directory_it(
            *validated_directory_path_exp,
            iterator_ec
        );
        if(iterator_ec){
            return std::unexpected(io_error::from_error_code(iterator_ec));
        }

        std::vector<bool> seen_orders(
            static_cast<std::size_t>(expected_testcase_count) + 1,
            false
        );
        std::int32_t testcase_count = 0;
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
                order > expected_testcase_count
            ){
                return std::unexpected(
                    judge_error{
                        judge_error_code::validation_error,
                        "invalid testcase output layout"
                    }
                );
            }

            if(seen_orders[static_cast<std::size_t>(order)]){
                return std::unexpected(
                    judge_error{
                        judge_error_code::validation_error,
                        "duplicate testcase output layout"
                    }
                );
            }

            seen_orders[static_cast<std::size_t>(order)] = true;
            ++testcase_count;
        }

        if(testcase_count != expected_testcase_count){
            return std::unexpected(
                judge_error{
                    judge_error_code::validation_error,
                    "testcase output layout does not match manifest"
                }
            );
        }

        for(std::int32_t order = 1; order <= expected_testcase_count; ++order){
            if(!seen_orders[static_cast<std::size_t>(order)]){
                return std::unexpected(
                    judge_error{
                        judge_error_code::validation_error,
                        "testcase output layout does not match manifest"
                    }
                );
            }
        }

        return {};
    }
}

std::expected<std::filesystem::path, io_error> snapshot_layout::make_problem_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(make_invalid_argument_error("invalid problem id"));
    }

    return make_validated_testcase_path(
        testcase_root_path,
        std::filesystem::path(std::to_string(problem_id))
    );
}

std::expected<std::filesystem::path, io_error> snapshot_layout::make_version_directory_path(
    const std::filesystem::path& testcase_root_path,
    std::int64_t problem_id,
    std::int32_t version
){
    if(version <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase version"));
    }

    const auto problem_directory_path_exp = make_problem_directory_path(
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

std::expected<std::filesystem::path, io_error> snapshot_layout::make_input_path(
    const std::filesystem::path& testcase_directory_path,
    std::int32_t order
){
    if(order <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase order"));
    }

    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".in"))
    );
}

std::expected<std::filesystem::path, io_error> snapshot_layout::make_output_path(
    const std::filesystem::path& testcase_directory_path,
    std::int32_t order
){
    if(order <= 0){
        return std::unexpected(make_invalid_argument_error("invalid testcase order"));
    }

    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path(format_testcase_file_name(order, ".out"))
    );
}

std::expected<std::filesystem::path, io_error> snapshot_layout::make_manifest_path(
    const std::filesystem::path& testcase_directory_path
){
    return make_validated_testcase_path(
        testcase_directory_path,
        std::filesystem::path("manifest.json")
    );
}

std::expected<void, judge_error> snapshot_layout::validate_materialized_snapshot(
    const std::filesystem::path& testcase_directory_path,
    const snapshot_manifest& manifest_value
){
    if(
        manifest_value.problem_id <= 0 ||
        manifest_value.version <= 0 ||
        manifest_value.testcase_count < 0
    ){
        return std::unexpected(
            judge_error{
                judge_error_code::validation_error,
                "invalid testcase snapshot manifest"
            }
        );
    }

    const auto manifest_path_exp = make_manifest_path(testcase_directory_path);
    if(!manifest_path_exp){
        return std::unexpected(manifest_path_exp.error());
    }

    const auto manifest_exists_exp = ensure_path_exists(
        *manifest_path_exp,
        "missing testcase snapshot manifest"
    );
    if(!manifest_exists_exp){
        return std::unexpected(manifest_exists_exp.error());
    }

    for(std::int32_t order = 1; order <= manifest_value.testcase_count; ++order){
        const auto input_path_exp = make_input_path(
            testcase_directory_path,
            order
        );
        if(!input_path_exp){
            return std::unexpected(input_path_exp.error());
        }

        const auto output_path_exp = make_output_path(
            testcase_directory_path,
            order
        );
        if(!output_path_exp){
            return std::unexpected(output_path_exp.error());
        }

        const auto input_exists_exp = ensure_path_exists(
            *input_path_exp,
            "missing testcase input file"
        );
        if(!input_exists_exp){
            return std::unexpected(input_exists_exp.error());
        }

        const auto output_exists_exp = ensure_path_exists(
            *output_path_exp,
            "missing testcase output file"
        );
        if(!output_exists_exp){
            return std::unexpected(output_exists_exp.error());
        }
    }

    return verify_testcase_output_layout(
        testcase_directory_path,
        manifest_value.testcase_count
    );
}
