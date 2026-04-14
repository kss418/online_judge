#include "db_service/testcase_mutation_service.hpp"

#include "db_service/testcase_bulk_mutation_service.hpp"
#include "db_service/testcase_item_mutation_service.hpp"

std::expected<problem_dto::testcase_mutation_result, service_error>
testcase_mutation_service::create_testcase(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::testcase& testcase_value
){
    return testcase_item_mutation_service::create_testcase(
        connection,
        problem_reference_value,
        testcase_value
    );
}

std::expected<problem_dto::testcase_mutation_result, service_error>
testcase_mutation_service::set_testcase_and_get(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    return testcase_item_mutation_service::set_testcase_and_get(
        connection,
        testcase_reference_value,
        testcase_value
    );
}

std::expected<problem_dto::mutation_result, service_error>
testcase_mutation_service::move_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value,
    std::int32_t target_testcase_order
){
    return testcase_item_mutation_service::move_testcase(
        connection,
        testcase_reference_value,
        target_testcase_order
    );
}

std::expected<problem_dto::mutation_result, service_error>
testcase_mutation_service::delete_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value
){
    return testcase_item_mutation_service::delete_testcase(
        connection,
        testcase_reference_value
    );
}

std::expected<problem_dto::mutation_result, service_error>
testcase_mutation_service::delete_all_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return testcase_bulk_mutation_service::delete_all_testcases(
        connection,
        problem_reference_value
    );
}

std::expected<problem_dto::testcase_count_mutation_result, service_error>
testcase_mutation_service::replace_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const std::vector<problem_dto::testcase>& testcase_values
){
    return testcase_bulk_mutation_service::replace_testcases(
        connection,
        problem_reference_value,
        testcase_values
    );
}
