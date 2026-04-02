#pragma once

#include "dto/problem_dto.hpp"

#include <vector>

namespace pqxx{
    class result;
    class row;
}

namespace problem_row_mapper{
    problem_dto::summary map_summary_row(const pqxx::row& problem_summary_row);
    std::vector<problem_dto::summary> map_summary_result(
        const pqxx::result& problem_summary_result
    );
}
