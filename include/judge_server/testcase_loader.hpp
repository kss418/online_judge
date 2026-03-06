#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

struct loaded_testcase{
    std::int32_t testcase_order = 0;
    std::filesystem::path input_file_path;
    std::filesystem::path output_file_path;
};

struct testcase_load_result{
    std::int64_t problem_id = 0;
    std::filesystem::path problem_directory_path;
    std::vector<loaded_testcase> loaded_testcases;
};

class testcase_loader{
public:
    static std::expected<testcase_loader, error_code> create(db_connection db_connection_value);
    std::expected<testcase_load_result, error_code> load(std::int64_t problem_id);
private:
    explicit testcase_loader(
        db_connection db_connection_value,
        std::filesystem::path testcase_root_path
    );

    std::expected<void, error_code> reset_problem_directory(
        const std::filesystem::path& problem_directory_path
    ) const;

    std::expected<loaded_testcase, error_code> save_testcase(
        const std::filesystem::path& problem_directory_path,
        const testcase& testcase_value
    ) const;

    static std::filesystem::path make_problem_directory_path(
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );

    static std::filesystem::path make_testcase_input_path(
        const std::filesystem::path& problem_directory_path,
        std::int32_t testcase_order
    );

    static std::filesystem::path make_testcase_output_path(
        const std::filesystem::path& problem_directory_path,
        std::int32_t testcase_order
    );

    db_connection db_connection_;
    std::filesystem::path testcase_root_path_;
};
