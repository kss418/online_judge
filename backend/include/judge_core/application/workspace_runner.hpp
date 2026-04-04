#pragma once

#include "error/judge_error.hpp"

#include <cstdint>
#include <expected>
#include <functional>
#include <filesystem>
#include <type_traits>
#include <utility>

class workspace_runner{
public:
    static std::expected<workspace_runner, judge_error> create(
        std::filesystem::path source_root_path
    );

    workspace_runner(workspace_runner&& other) noexcept;
    workspace_runner& operator=(workspace_runner&& other) noexcept;
    ~workspace_runner();

    workspace_runner(const workspace_runner&) = delete;
    workspace_runner& operator=(const workspace_runner&) = delete;

    template<typename Callback>
    auto with_submission_workspace(
        std::int64_t submission_id,
        Callback&& callback
    ) -> std::invoke_result_t<Callback, const std::filesystem::path&> {
        using result_type =
            std::invoke_result_t<Callback, const std::filesystem::path&>;

        const auto workspace_path_exp = prepare_workspace(submission_id);
        if(!workspace_path_exp){
            return std::unexpected(workspace_path_exp.error());
        }

        auto callback_result =
            std::forward<Callback>(callback)(*workspace_path_exp);
        const auto cleanup_workspace_exp = cleanup_workspace(*workspace_path_exp);
        if(!cleanup_workspace_exp){
            return std::unexpected(cleanup_workspace_exp.error());
        }

        return callback_result;
    }

private:
    explicit workspace_runner(
        std::filesystem::path source_root_path
    );

    std::expected<std::filesystem::path, judge_error> prepare_workspace(
        std::int64_t submission_id
    );
    std::expected<void, judge_error> cleanup_workspace(
        const std::filesystem::path& workspace_path
    );

    std::filesystem::path source_root_path_;
};
