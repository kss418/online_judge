#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <utility>

class tc_downloader{
public:
    static std::expected<tc_downloader, error_code> create(db_connection connection);
    std::expected<void, error_code> sync_tc(std::int64_t problem_id);

private:
    std::expected<bool, error_code> is_latest(std::int64_t problem_id);
    std::expected<void, error_code> sync_version_file(std::int64_t problem_id);
    std::expected<void, error_code> sync_limit_file(std::int64_t problem_id);

    std::expected<void, error_code> delete_outdated(std::int64_t problem_id);
    std::expected<void, error_code> delete_one(
        std::int64_t problem_id,
        std::int32_t order
    );

    std::expected<void, error_code> download_all(std::int64_t problem_id);
    std::expected<void, error_code> download_one(
        std::int64_t problem_id,
        std::int32_t order
    );
    
    explicit tc_downloader(db_connection connection);
    std::expected<std::int32_t, error_code> read_version_file(std::int64_t problem_id) const;
    std::expected<std::pair<std::int32_t, std::int32_t>, error_code> read_limit_file(
        std::int64_t problem_id
    ) const;

    db_connection connection_;
};
