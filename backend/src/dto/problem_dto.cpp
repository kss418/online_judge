#include "dto/problem_dto.hpp"

bool problem_dto::is_valid(const reference& reference_value){
    return reference_value.problem_id > 0;
}

bool problem_dto::is_valid(const title& title_value){
    return !title_value.value.empty();
}

bool problem_dto::is_valid(const create_request& create_request_value){
    return !create_request_value.title.empty();
}

bool problem_dto::is_valid(const update_request& update_request_value){
    return !update_request_value.title.empty();
}

bool problem_dto::is_valid(const testcase_ref& testcase_reference_value){
    return
        testcase_reference_value.problem_id > 0 &&
        testcase_reference_value.testcase_order > 0;
}
