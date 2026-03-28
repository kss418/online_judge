#include "serializer/user_json_serializer.hpp"

#include "common/permission_util.hpp"

#include <utility>

namespace{
    boost::json::object make_summary_object(
        const auth_dto::user_summary& user_summary_value
    ){
        boost::json::object response_object;
        response_object["user_id"] = user_summary_value.user_id;
        response_object["user_name"] = user_summary_value.user_name;
        if(user_summary_value.user_login_id_opt){
            response_object["user_login_id"] = *user_summary_value.user_login_id_opt;
        }
        else{
            response_object["user_login_id"] = nullptr;
        }
        response_object["permission_level"] = user_summary_value.permission_level;
        response_object["role_name"] = permission_util::role_name(
            user_summary_value.permission_level
        );
        response_object["created_at"] = user_summary_value.created_at;
        return response_object;
    }

    boost::json::array make_summary_array(
        const auth_dto::user_summary_list& user_summary_values
    ){
        boost::json::array response_array;
        response_array.reserve(user_summary_values.size());
        for(const auto& user_summary_value : user_summary_values){
            response_array.push_back(make_summary_object(user_summary_value));
        }
        return response_array;
    }
}

boost::json::object user_json_serializer::make_permission_object(
    std::int64_t user_id,
    std::int32_t permission_level
){
    boost::json::object response_object;
    response_object["user_id"] = user_id;
    response_object["permission_level"] = permission_level;
    response_object["role_name"] = permission_util::role_name(permission_level);
    return response_object;
}

boost::json::object user_json_serializer::make_me_object(
    const auth_dto::identity& auth_identity_value
){
    boost::json::object response_object;
    response_object["id"] = auth_identity_value.user_id;
    response_object["user_name"] = auth_identity_value.user_name;
    response_object["permission_level"] = auth_identity_value.permission_level;
    response_object["role_name"] = permission_util::role_name(
        auth_identity_value.permission_level
    );
    return response_object;
}

boost::json::object user_json_serializer::make_submission_statistics_object(
    const user_statistics_dto::submission_statistics& statistics_value
){
    boost::json::object response_object;
    response_object["user_id"] = statistics_value.user_id;
    response_object["submission_count"] = statistics_value.submission_count;
    response_object["queued_submission_count"] = statistics_value.queued_submission_count;
    response_object["judging_submission_count"] = statistics_value.judging_submission_count;
    response_object["accepted_submission_count"] = statistics_value.accepted_submission_count;
    response_object["wrong_answer_submission_count"] =
        statistics_value.wrong_answer_submission_count;
    response_object["time_limit_exceeded_submission_count"] =
        statistics_value.time_limit_exceeded_submission_count;
    response_object["memory_limit_exceeded_submission_count"] =
        statistics_value.memory_limit_exceeded_submission_count;
    response_object["runtime_error_submission_count"] =
        statistics_value.runtime_error_submission_count;
    response_object["compile_error_submission_count"] =
        statistics_value.compile_error_submission_count;
    response_object["output_exceeded_submission_count"] =
        statistics_value.output_exceeded_submission_count;
    if(statistics_value.last_submission_at_opt){
        response_object["last_submission_at"] = *statistics_value.last_submission_at_opt;
    }
    else{
        response_object["last_submission_at"] = nullptr;
    }
    if(statistics_value.last_accepted_at_opt){
        response_object["last_accepted_at"] = *statistics_value.last_accepted_at_opt;
    }
    else{
        response_object["last_accepted_at"] = nullptr;
    }
    response_object["updated_at"] = statistics_value.updated_at;
    return response_object;
}

boost::json::object user_json_serializer::make_list_object(
    const auth_dto::user_summary_list& user_summary_values
){
    boost::json::object response_object;
    response_object["user_count"] = static_cast<std::int64_t>(user_summary_values.size());
    response_object["users"] = make_summary_array(user_summary_values);
    return response_object;
}
