#pragma once

#include "dto/auth_dto.hpp"

#include <boost/json.hpp>

namespace auth_json_serializer{
    boost::json::object make_session_object(const auth_dto::session& session_value);
}
