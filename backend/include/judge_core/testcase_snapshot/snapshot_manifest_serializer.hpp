#pragma once

#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot/snapshot_manifest.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace snapshot_manifest_serializer{
    std::expected<std::string, judge_error> serialize(
        const snapshot_manifest& manifest_value
    );

    std::expected<snapshot_manifest, judge_error> parse(
        std::string_view manifest_content
    );
}
