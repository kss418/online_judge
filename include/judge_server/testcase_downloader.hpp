#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>

class testcase_downloader{
public:
    static std::expected<testcase_downloader, error_code> create(db_connection connection);
    std::expected<void, error_code> download_one(
        std::int64_t problem_id,
        std::int32_t order
    );

private:
    explicit testcase_downloader(
        db_connection connection,
        std::filesystem::path root_path
    );
    std::filesystem::path make_input_path(
        std::int64_t problem_id,
        std::int32_t order
    ) const;
    std::filesystem::path make_output_path(
        std::int64_t problem_id,
        std::int32_t order
    ) const;

    db_connection connection_;
    std::filesystem::path root_path_;
};
