#include "common/submission_status.hpp"

std::string to_string(submission_status status){
    switch(status){
        case submission_status::queued:
            return "queued";
        case submission_status::judging:
            return "judging";
        case submission_status::accepted:
            return "accepted";
        case submission_status::wrong_answer:
            return "wrong_answer";
        case submission_status::time_limit_exceeded:
            return "time_limit_exceeded";
        case submission_status::memory_limit_exceeded:
            return "memory_limit_exceeded";
        case submission_status::runtime_error:
            return "runtime_error";
        case submission_status::compile_error:
            return "compile_error";
        case submission_status::build_resource_exceeded:
            return "build_resource_exceeded";
        case submission_status::output_exceeded:
            return "output_exceeded";
        case submission_status::infra_failure:
            return "infra_failure";
    }

    return "queued";
}

std::optional<submission_status> parse_submission_status(std::string_view status){
    if(status == "queued"){
        return submission_status::queued;
    }
    if(status == "judging"){
        return submission_status::judging;
    }
    if(status == "accepted"){
        return submission_status::accepted;
    }
    if(status == "wrong_answer"){
        return submission_status::wrong_answer;
    }
    if(status == "time_limit_exceeded"){
        return submission_status::time_limit_exceeded;
    }
    if(status == "memory_limit_exceeded"){
        return submission_status::memory_limit_exceeded;
    }
    if(status == "runtime_error"){
        return submission_status::runtime_error;
    }
    if(status == "compile_error"){
        return submission_status::compile_error;
    }
    if(status == "build_resource_exceeded"){
        return submission_status::build_resource_exceeded;
    }
    if(status == "output_exceeded"){
        return submission_status::output_exceeded;
    }
    if(status == "infra_failure"){
        return submission_status::infra_failure;
    }

    return std::nullopt;
}
