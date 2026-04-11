#pragma once

#include <cstdint>
#include <string>

namespace submission_domain_dto{
    struct leased_submission{
        std::int64_t submission_id = 0;
        std::int64_t problem_id = 0;
        std::int32_t problem_version = 0;
        std::int64_t queue_wait_ms = 0;
        std::int32_t attempt_no = 0;
        std::string lease_token;
        std::string leased_until;
        std::string language;
        std::string source_code;
    };
}
