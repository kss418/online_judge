#pragma once

#include "dto/auth_dto.hpp"
#include "dto/user_statistics_dto.hpp"

#include <boost/json.hpp>

#include <cstdint>

namespace user_json_serializer{
    boost::json::object make_permission_object(
        std::int64_t user_id,
        std::int32_t permission_level
    );

    boost::json::object make_me_object(const auth_dto::identity& auth_identity_value);

    boost::json::object make_submission_statistics_object(
        const user_statistics_dto::submission_statistics& statistics_value
    );

    boost::json::object make_list_object(
        const auth_dto::user_summary_list& user_summary_values
    );
}
