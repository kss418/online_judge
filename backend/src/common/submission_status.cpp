#include "common/submission_status.hpp"

std::string to_string(submission_status status){
    const auto* spec = find_submission_status_spec(status);
    if(spec != nullptr){
        return std::string{spec->code};
    }

    return "queued";
}

std::optional<submission_status> parse_submission_status(std::string_view status){
    for(const auto& spec : all_submission_status_specs()){
        if(spec.code == status){
            return spec.status;
        }
    }

    return std::nullopt;
}
