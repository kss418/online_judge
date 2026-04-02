#pragma once

#include "query_builder/assembled_query.hpp"
#include "dto/submission_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace submission_query_builder{
    class submission_list_query_builder{
    public:
        submission_list_query_builder(
            const submission_dto::list_filter& filter_value,
            std::optional<std::int64_t> viewer_user_id_opt
        );

        std::expected<assembled_query, repository_error> build_list_query() const;

    private:
        const submission_dto::list_filter& filter_value_;
        std::optional<std::int64_t> viewer_user_id_opt_;
    };

    std::expected<assembled_query, repository_error> build_status_snapshot_query(
        const std::vector<std::int64_t>& submission_ids
    );
}
