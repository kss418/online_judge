#include "common/error_code.hpp"
#include "judge_server/judge.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
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

int main(int argc, char** argv){
    if(argc < 4 || argc > 7){
        std::cerr
            << "usage: judge_server <source_path> <input_path> <answer_path> "
            << "[compiler_path] [time_limit_ms] [memory_limit_mb]\n";
        return 1;
    }

    const std::filesystem::path source_path{argv[1]};
    const std::filesystem::path input_path{argv[2]};
    const std::filesystem::path answer_path{argv[3]};
    const std::filesystem::path compiler_path{argc >= 5 ? argv[4] : "/usr/bin/g++"};

    std::chrono::milliseconds time_limit{2000};
    std::int64_t memory_limit_mb = 256;

    if(argc >= 6){
        auto time_limit_exp = parse_positive_int64(argv[5]);
        if(!time_limit_exp){
            std::cerr << "invalid time_limit_ms: " << argv[5] << '\n';
            return 1;
        }
        time_limit = std::chrono::milliseconds{*time_limit_exp};
    }

    if(argc >= 7){
        auto memory_limit_exp = parse_positive_int64(argv[6]);
        if(!memory_limit_exp){
            std::cerr << "invalid memory_limit_mb: " << argv[6] << '\n';
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
