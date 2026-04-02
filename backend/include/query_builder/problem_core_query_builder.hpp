#pragma once

#include "query_builder/assembled_query.hpp"
#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>

namespace problem_core_query_builder{
    class problem_list_query_builder{
    public:
        problem_list_query_builder(
            const problem_dto::list_filter& filter_value,
            std::optional<std::int64_t> viewer_user_id_opt
        );

        std::expected<assembled_query, repository_error> build_list_query() const;
        std::expected<assembled_query, repository_error> build_count_query() const;

    private:
        const problem_dto::list_filter& filter_value_;
        std::optional<std::int64_t> viewer_user_id_opt_;
    };

    class user_problem_list_query_builder{
    public:
        user_problem_list_query_builder(
            std::int64_t user_id,
            std::optional<std::int64_t> viewer_user_id_opt
        );

        std::expected<assembled_query, repository_error> build_solved_query() const;
        std::expected<assembled_query, repository_error> build_wrong_query() const;

    private:
        std::int64_t user_id_;
        std::optional<std::int64_t> viewer_user_id_opt_;
    };
}
