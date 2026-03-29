#include "serializer/user_json_serializer.hpp"

#include "common/permission_util.hpp"

#include <utility>

namespace{
    boost::json::object make_public_list_item_object(
        const user_dto::list_item& user_value
    ){
        boost::json::object response_object;
        response_object["user_id"] = user_value.user_id;
        response_object["user_login_id"] = user_value.user_login_id;
        response_object["solved_problem_count"] = user_value.solved_problem_count;
        response_object["accepted_submission_count"] = user_value.accepted_submission_count;
        response_object["submission_count"] = user_value.submission_count;
        response_object["created_at"] = user_value.created_at;
        return response_object;
    }

    boost::json::array make_public_list_item_array(
        const user_dto::list& user_values
    ){
        boost::json::array response_array;
        response_array.reserve(user_values.size());
        for(const auto& user_value : user_values){
            response_array.push_back(make_public_list_item_object(user_value));
        }
        return response_array;
    }

    boost::json::object make_summary_object(
        const auth_dto::user_summary& user_summary_value
    ){
        boost::json::object response_object;
        response_object["user_id"] = user_summary_value.user_id;
        response_object["user_login_id"] = user_summary_value.user_login_id;
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

    boost::json::object make_problem_summary_object(
        const problem_dto::summary& summary_value
    ){
        boost::json::object response_object;
        response_object["problem_id"] = summary_value.problem_id;
        response_object["title"] = summary_value.title;
        response_object["version"] = summary_value.version;
        response_object["submission_count"] = summary_value.submission_count;
        response_object["accepted_count"] = summary_value.accepted_count;
        if(summary_value.user_problem_state_opt){
            response_object["user_problem_state"] = *summary_value.user_problem_state_opt;
        }
        else{
            response_object["user_problem_state"] = nullptr;
        }
        return response_object;
    }

    boost::json::array make_problem_summary_array(
        const std::vector<problem_dto::summary>& summary_values
    ){
        boost::json::array response_array;
        response_array.reserve(summary_values.size());
        for(const auto& summary_value : summary_values){
            response_array.push_back(make_problem_summary_object(summary_value));
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

boost::json::object user_json_serializer::make_submission_ban_object(
    const user_dto::submission_ban& submission_ban_value
){
    boost::json::object response_object;
    response_object["user_id"] = submission_ban_value.user_id;
    response_object["duration_minutes"] = submission_ban_value.duration_minutes;
    response_object["submission_banned_until"] = submission_ban_value.submission_banned_until;
    return response_object;
}

boost::json::object user_json_serializer::make_me_object(
    const auth_dto::identity& auth_identity_value
){
    boost::json::object response_object;
    response_object["id"] = auth_identity_value.user_id;
    response_object["user_login_id"] = auth_identity_value.user_login_id;
    response_object["permission_level"] = auth_identity_value.permission_level;
    response_object["role_name"] = permission_util::role_name(
        auth_identity_value.permission_level
    );
    return response_object;
}

boost::json::object user_json_serializer::make_summary_object(
    const user_dto::summary& summary_value
){
    boost::json::object response_object;
    response_object["user_id"] = summary_value.user_id;
    response_object["user_login_id"] = summary_value.user_login_id;
    response_object["created_at"] = summary_value.created_at;
    return response_object;
}

boost::json::object user_json_serializer::make_public_list_object(
    const user_dto::list& user_values
){
    boost::json::object response_object;
    response_object["user_count"] = static_cast<std::int64_t>(user_values.size());
    response_object["users"] = make_public_list_item_array(user_values);
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

boost::json::object user_json_serializer::make_solved_problem_list_object(
    const std::vector<problem_dto::summary>& solved_problem_values
){
    boost::json::object response_object;
    response_object["solved_problem_count"] =
        static_cast<std::int64_t>(solved_problem_values.size());
    response_object["solved_problems"] = make_problem_summary_array(solved_problem_values);
    return response_object;
}

boost::json::object user_json_serializer::make_wrong_problem_list_object(
    const std::vector<problem_dto::summary>& wrong_problem_values
){
    boost::json::object response_object;
    response_object["wrong_problem_count"] =
        static_cast<std::int64_t>(wrong_problem_values.size());
    response_object["wrong_problems"] = make_problem_summary_array(wrong_problem_values);
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
