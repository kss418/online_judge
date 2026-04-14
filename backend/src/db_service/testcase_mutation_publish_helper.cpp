#include "db_service/testcase_mutation_publish_helper.hpp"

#include "db_service/problem_version_publish_service.hpp"

#include "db_repository/problem_query_repository.hpp"

std::expected<problem_dto::version, service_error>
testcase_mutation_publish_helper::increase_version_and_publish_current_snapshot(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    return problem_version_publish_service::increase_version_and_publish_current_snapshot(
        transaction,
        problem_reference_value
    );
}

std::expected<problem_dto::version, service_error>
testcase_mutation_publish_helper::get_current_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    return problem_query_repository::get_version(
        transaction,
        problem_reference_value
    );
}

problem_dto::mutation_result testcase_mutation_publish_helper::make_mutation_result(
    const problem_dto::reference& problem_reference_value,
    const problem_dto::version& version_value
){
    return problem_dto::mutation_result{
        .problem_id = problem_reference_value.problem_id,
        .version = version_value.version
    };
}

problem_dto::testcase_mutation_result
testcase_mutation_publish_helper::make_testcase_mutation_result(
    const problem_dto::testcase& testcase_value,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::version& version_value
){
    return problem_dto::testcase_mutation_result{
        .testcase_value = testcase_value,
        .problem_value = make_mutation_result(
            problem_reference_value,
            version_value
        )
    };
}

problem_dto::testcase_count_mutation_result
testcase_mutation_publish_helper::make_testcase_count_mutation_result(
    const problem_dto::testcase_count& testcase_count_value,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::version& version_value
){
    return problem_dto::testcase_count_mutation_result{
        .testcase_count_value = testcase_count_value,
        .problem_value = make_mutation_result(
            problem_reference_value,
            version_value
        )
    };
}
