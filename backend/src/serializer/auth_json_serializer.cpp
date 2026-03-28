#include "serializer/auth_json_serializer.hpp"

#include "common/permission_util.hpp"

boost::json::object auth_json_serializer::make_session_object(
    const auth_dto::session& session_value
){
    boost::json::object response_object;
    response_object["user_id"] = session_value.user_id;
    response_object["permission_level"] = session_value.permission_level;
    response_object["role_name"] = permission_util::role_name(session_value.permission_level);
    response_object["user_name"] = session_value.user_name;
    response_object["token"] = session_value.token;
    return response_object;
}
