#pragma once

#include "dto/submission_dto.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace param_util{
    std::optional<std::string> try_apply_submission_list_filter(
        std::string_view key,
        std::string_view value,
        submission_dto::list_filter& filter_value
    );
}
