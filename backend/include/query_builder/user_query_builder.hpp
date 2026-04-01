#pragma once

#include "query_builder/assembled_query.hpp"
#include "dto/user_dto.hpp"

#include <expected>

namespace user_query_builder{
    class public_user_list_query_builder{
    public:
        explicit public_user_list_query_builder(const user_dto::list_filter& filter_value);

        assembled_query build() const;

    private:
        const user_dto::list_filter& filter_value_;
    };
}
