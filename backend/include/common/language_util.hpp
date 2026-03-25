#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace language_util{
    struct supported_language{
        std::string_view language;
        std::string_view source_extension;
    };

    std::span<const supported_language> list_supported_languages();
    std::optional<supported_language> find_supported_language(std::string_view language);
    std::optional<supported_language> find_supported_language_by_extension(
        std::string_view source_extension
    );
}
