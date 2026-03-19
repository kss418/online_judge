#include "http_server/param_util.hpp"

#include "common/string_util.hpp"
#include "db/submission_util.hpp"

std::optional<std::string> param_util::try_apply_submission_list_filter(
    std::string_view key,
    std::string_view value,
    submission_dto::list_filter& filter_value
){
    if(key == "top"){
        if(filter_value.top_submission_id_opt){
            return "duplicate query parameter: top";
        }

        const auto top_submission_id_opt = string_util::parse_positive_int64(value);
        if(!top_submission_id_opt){
            return "invalid query parameter: top";
        }

        filter_value.top_submission_id_opt = *top_submission_id_opt;
        return std::nullopt;
    }
    if(key == "user_id"){
        if(filter_value.user_id_opt){
            return "duplicate query parameter: user_id";
        }

        const auto user_id_opt = string_util::parse_positive_int64(value);
        if(!user_id_opt){
            return "invalid query parameter: user_id";
        }

        filter_value.user_id_opt = *user_id_opt;
        return std::nullopt;
    }
    if(key == "problem_id"){
        if(filter_value.problem_id_opt){
            return "duplicate query parameter: problem_id";
        }

        const auto problem_id_opt = string_util::parse_positive_int64(value);
        if(!problem_id_opt){
            return "invalid query parameter: problem_id";
        }

        filter_value.problem_id_opt = *problem_id_opt;
        return std::nullopt;
    }
    if(key == "status"){
        if(filter_value.status_opt){
            return "duplicate query parameter: status";
        }

        const auto status_opt = parse_submission_status(value);
        if(!status_opt){
            return "invalid query parameter: status";
        }

        filter_value.status_opt = to_string(*status_opt);
        return std::nullopt;
    }

    return "unsupported query parameter: " + std::string{key};
}
