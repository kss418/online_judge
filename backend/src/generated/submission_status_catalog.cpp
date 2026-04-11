#include "common/submission_status.hpp"

#include <array>
#include <span>

namespace{
    constexpr std::array<submission_status_spec, 11> submission_status_specs{{
        submission_status_spec{
            submission_status::queued,
            "queued",
            "queued",
            "대기 중",
            "neutral",
            "queued",
            false,
            false,
            true,
            false
        },
        submission_status_spec{
            submission_status::judging,
            "judging",
            "judging",
            "채점 중",
            "info",
            "judging",
            false,
            false,
            true,
            false
        },
        submission_status_spec{
            submission_status::accepted,
            "accepted",
            "accepted",
            "정답",
            "success",
            "accepted",
            true,
            false,
            true,
            true
        },
        submission_status_spec{
            submission_status::wrong_answer,
            "wrong_answer",
            "wrong_answer",
            "오답",
            "danger",
            "wrong_answer",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::time_limit_exceeded,
            "time_limit_exceeded",
            "time_limit_exceeded",
            "시간 초과",
            "warning",
            "time_limit_exceeded",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::memory_limit_exceeded,
            "memory_limit_exceeded",
            "memory_limit_exceeded",
            "메모리 초과",
            "warning",
            "memory_limit_exceeded",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::runtime_error,
            "runtime_error",
            "runtime_error",
            "런타임 에러",
            "danger",
            "runtime_error",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::compile_error,
            "compile_error",
            "compile_error",
            "컴파일 에러",
            "info",
            "compile_error",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::build_resource_exceeded,
            "build_resource_exceeded",
            "build_resource_exceeded",
            "빌드 리소스 초과",
            "warning",
            "build_resource_exceeded",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::output_exceeded,
            "output_exceeded",
            "output_exceeded",
            "출력 초과",
            "warning",
            "output_exceeded",
            true,
            true,
            true,
            true
        },
        submission_status_spec{
            submission_status::infra_failure,
            "infra_failure",
            "infra_failure",
            "인프라 실패",
            "danger",
            "infra_failure",
            true,
            true,
            true,
            true
        }
    }};
}

std::span<const submission_status_spec> all_submission_status_specs(){
    return submission_status_specs;
}

const submission_status_spec* find_submission_status_spec(submission_status status){
    for(const auto& spec : submission_status_specs){
        if(spec.status == status){
            return &spec;
        }
    }

    return nullptr;
}

