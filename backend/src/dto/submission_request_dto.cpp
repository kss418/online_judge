#include "dto/submission_request_dto.hpp"

#include "common/language_util.hpp"

bool submission_request_dto::is_valid(const submit_request& submit_request_value){
    return
        !submit_request_value.language.empty() &&
        !submit_request_value.source_code.empty() &&
        language_util::find_supported_language(submit_request_value.language).has_value();
}
