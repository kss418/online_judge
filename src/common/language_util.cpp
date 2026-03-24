#include "common/language_util.hpp"

#include <array>

static constexpr std::array<language_util::supported_language, 3> SUPPORTED_LANGUAGES{{
    {"cpp", ".cpp"},
    {"python", ".py"},
    {"java", ".java"},
}};

std::span<const language_util::supported_language> language_util::list_supported_languages(){
    return SUPPORTED_LANGUAGES;
}

std::optional<language_util::supported_language> language_util::find_supported_language(
    std::string_view language
){
    for(const auto supported_language_value : list_supported_languages()){
        if(supported_language_value.language == language){
            return supported_language_value;
        }
    }

    return std::nullopt;
}

std::optional<language_util::supported_language> language_util::find_supported_language_by_extension(
    std::string_view source_extension
){
    for(const auto supported_language_value : list_supported_languages()){
        if(supported_language_value.source_extension == source_extension){
            return supported_language_value;
        }
    }

    return std::nullopt;
}
