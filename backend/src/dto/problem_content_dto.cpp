#include "dto/problem_content_dto.hpp"

bool problem_content_dto::is_valid(const limits& limits_value){
    return limits_value.memory_mb > 0 && limits_value.time_ms > 0;
}

bool problem_content_dto::is_valid(const statement& statement_value){
    return
        !statement_value.description.empty() &&
        !statement_value.input_format.empty() &&
        !statement_value.output_format.empty();
}

bool problem_content_dto::is_valid(const sample_ref& sample_reference_value){
    return sample_reference_value.problem_id > 0 && sample_reference_value.sample_order > 0;
}
