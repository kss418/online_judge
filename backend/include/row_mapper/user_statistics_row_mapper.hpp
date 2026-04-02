#pragma once

#include "dto/user_statistics_dto.hpp"

namespace pqxx{
    class row;
}

namespace user_statistics_row_mapper{
    user_statistics_dto::submission_statistics map_submission_statistics_row(
        const pqxx::row& user_submission_statistics_row
    );
}
