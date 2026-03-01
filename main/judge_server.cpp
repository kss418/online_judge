#include "common/error_code.hpp"
#include "judge_server/judge.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

std::expected<std::int64_t, error_code> parse_positive_int64(std::string_view text){
    if(text.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::int64_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    auto parse_exp = std::from_chars(begin, end, value);
    if(parse_exp.ec != std::errc() || parse_exp.ptr != end || value <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return value;
}

std::string judge_result_to_string(judge_result result){
    switch(result){
        case judge_result::accepted:
            return "accepted";
        case judge_result::wrong_answer:
            return "wrong_answer";
        case judge_result::time_limit_excced:
            return "time_limit_exceeded";
        case judge_result::memory_limit_excced:
            return "memory_limit_exceeded";
        case judge_result::runtime_error:
            return "runtime_error";
        case judge_result::compile_error:
            return "compile_error";
        case judge_result::output_exceed:
            return "output_exceed";
        case judge_result::invalid_output:
            return "invalid_output";
    }
    return "unknown_result";
}

std::expected<std::string, error_code> get_required_env(const char* key){
    const char* value = std::getenv(key);
    if(value == nullptr || std::string_view{value}.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::string{value};
}

int main(){
    auto source_path_text_exp = get_required_env("JUDGE_SOURCE_PATH");
    if(!source_path_text_exp){
        std::cerr << "JUDGE_SOURCE_PATH environment variable is missing\n";
        return 1;
    }

    auto input_path_text_exp = get_required_env("JUDGE_INPUT_PATH");
    if(!input_path_text_exp){
        std::cerr << "JUDGE_INPUT_PATH environment variable is missing\n";
        return 1;
    }

    auto answer_path_text_exp = get_required_env("JUDGE_ANSWER_PATH");
    if(!answer_path_text_exp){
        std::cerr << "JUDGE_ANSWER_PATH environment variable is missing\n";
        return 1;
    }

    const std::filesystem::path source_path{*source_path_text_exp};
    const std::filesystem::path input_path{*input_path_text_exp};
    const std::filesystem::path answer_path{*answer_path_text_exp};

    const char* compiler_path_text = std::getenv("JUDGE_COMPILER_PATH");
    const std::filesystem::path compiler_path{
        compiler_path_text != nullptr && !std::string_view{compiler_path_text}.empty()
            ? compiler_path_text
            : "/usr/bin/g++"
    };

    std::chrono::milliseconds time_limit{2000};
    std::int64_t memory_limit_mb = 256;

    const char* time_limit_text = std::getenv("JUDGE_TIME_LIMIT_MS");
    if(time_limit_text != nullptr && !std::string_view{time_limit_text}.empty()){
        auto time_limit_exp = parse_positive_int64(time_limit_text);
        if(!time_limit_exp){
            std::cerr << "invalid JUDGE_TIME_LIMIT_MS: " << time_limit_text << '\n';
            return 1;
        }
        time_limit = std::chrono::milliseconds{*time_limit_exp};
    }

    const char* memory_limit_text = std::getenv("JUDGE_MEMORY_LIMIT_MB");
    if(memory_limit_text != nullptr && !std::string_view{memory_limit_text}.empty()){
        auto memory_limit_exp = parse_positive_int64(memory_limit_text);
        if(!memory_limit_exp){
            std::cerr << "invalid JUDGE_MEMORY_LIMIT_MB: " << memory_limit_text << '\n';
            return 1;
        }
        memory_limit_mb = *memory_limit_exp;
    }

    auto judge_exp = judge::judge_cpp(
        source_path, input_path, answer_path, compiler_path, time_limit, memory_limit_mb
    );
    if(!judge_exp){
        std::cerr << "judge failed: " << to_string(judge_exp.error()) << '\n';
        return 1;
    }

    std::cout << judge_result_to_string(judge_exp->first) << '\n';
    if(!judge_exp->second.empty()){
        std::cout << judge_exp->second << '\n';
    }

    return judge_exp->first == judge_result::accepted ? 0 : 2;
}
