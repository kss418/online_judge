#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"

#include <expected>
#include <filesystem>
#include <string_view>

class judge_worker{
public:
    static std::expected<judge_worker, error_code> create(submission_service submission_service);

    std::expected<bool, error_code> save_source_code();

private:
    explicit judge_worker(
        submission_service submission_service,
        std::filesystem::path source_root_path
    );

    static bool is_queue_empty_error(const error_code& code);
    std::expected<void, error_code> write_source_code(
        const std::filesystem::path& source_path,
        std::string_view source_code
    ) const;
    std::filesystem::path make_source_file_path(
        std::int64_t submission_id,
        std::string_view language
    ) const;

    submission_service submission_service_;
    std::filesystem::path source_root_path_;
};
