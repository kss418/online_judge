#pragma once

#include <optional>
#include <string>

struct dto_validation_error{
    std::string code;
    std::string message;
    std::optional<std::string> field_opt = std::nullopt;
};
