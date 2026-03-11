#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class judge_result{
    accepted,
    wrong_answer,
    time_limit_excced,
    memory_limit_excced,
    runtime_error,
    compile_error,
    output_exceed,
    invalid_output
};

class judge_util{
public:
    static judge_util& instance();

    std::expected<std::filesystem::path, error_code> make_source_directory_path();
    std::expected<std::filesystem::path, error_code> make_source_file_path(
        std::int64_t submission_id,
        std::string_view language
    );

    std::vector<std::string> normalize_output(const std::string& output);

private:
    judge_util() = default;

    void initialize_if_needed();
    bool is_blank(char c);

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> source_directory_path_;
};
