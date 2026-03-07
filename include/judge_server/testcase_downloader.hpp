#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <expected>
#include <filesystem>

class testcase_downloader{
public:
    static std::expected<testcase_downloader, error_code> create(db_connection connection);

private:
    explicit testcase_downloader(
        db_connection connection,
        std::filesystem::path root_path
    );

    db_connection connection_;
    std::filesystem::path root_path_;
};
