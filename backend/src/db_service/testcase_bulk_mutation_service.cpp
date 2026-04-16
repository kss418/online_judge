#include "db_service/testcase_bulk_mutation_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_service/testcase_mutation_publish_helper.hpp"

#include "db_repository/problem_content_repository.hpp"
#include "db_repository/testcase_mutation_repository.hpp"
#include "db_repository/testcase_query_repository.hpp"

namespace{
    std::expected<void, service_error> delete_all_testcases_and_clear_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    ){
        const auto delete_all_testcases_exp = testcase_mutation_repository::delete_all_testcases(
            transaction,
            problem_reference_value
        );
        if(!delete_all_testcases_exp){
            return std::unexpected(delete_all_testcases_exp.error());
        }

        const auto clear_testcase_count_exp = testcase_mutation_repository::clear_testcase_count(
            transaction,
            problem_reference_value
        );
        if(!clear_testcase_count_exp){
            return std::unexpected(clear_testcase_count_exp.error());
        }

        return {};
    }
}

std::expected<problem_dto::mutation_result, service_error>
testcase_bulk_mutation_service::delete_all_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto testcase_count_exp = testcase_query_repository::get_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!testcase_count_exp){
                return std::unexpected(testcase_count_exp.error());
            }

            if(testcase_count_exp->testcase_count <= 0){
                return testcase_mutation_publish_helper::with_current_version(
                    transaction,
                    problem_reference_value,
                    [&](const problem_dto::version& version_value)
                        -> std::expected<problem_dto::mutation_result, service_error> {
                        return testcase_mutation_publish_helper::make_mutation_result(
                            problem_reference_value,
                            version_value
                        );
                    }
                );
            }

            const auto clear_testcases_exp = delete_all_testcases_and_clear_count(
                transaction,
                problem_reference_value
            );
            if(!clear_testcases_exp){
                return std::unexpected(clear_testcases_exp.error());
            }

            return testcase_mutation_publish_helper::with_published_version(
                transaction,
                problem_reference_value,
                [&](const problem_dto::version& version_value)
                    -> std::expected<problem_dto::mutation_result, service_error> {
                    return testcase_mutation_publish_helper::make_mutation_result(
                        problem_reference_value,
                        version_value
                    );
                }
            );
        }
    );
}

std::expected<problem_dto::testcase_count_mutation_result, service_error>
testcase_bulk_mutation_service::replace_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const std::vector<problem_dto::testcase>& testcase_values
){
    if(testcase_values.size() > 999){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::testcase_count_mutation_result, service_error> {
            const auto ensure_statement_exp = problem_content_repository::ensure_statement_row(
                transaction,
                problem_reference_value
            );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            const auto current_testcase_count_exp =
                testcase_query_repository::get_testcase_count(
                    transaction,
                    problem_reference_value
                );
            if(!current_testcase_count_exp){
                return std::unexpected(current_testcase_count_exp.error());
            }
            const bool has_changes =
                current_testcase_count_exp->testcase_count > 0 || !testcase_values.empty();

            if(current_testcase_count_exp->testcase_count > 0){
                const auto clear_testcases_exp = delete_all_testcases_and_clear_count(
                    transaction,
                    problem_reference_value
                );
                if(!clear_testcases_exp){
                    return std::unexpected(clear_testcases_exp.error());
                }
            }

            problem_dto::testcase_count testcase_count_value{
                .testcase_count = 0
            };
            for(const auto& testcase_value : testcase_values){
                const auto next_testcase_count_exp =
                    testcase_mutation_repository::increase_testcase_count(
                        transaction,
                        problem_reference_value
                    );
                if(!next_testcase_count_exp){
                    return std::unexpected(next_testcase_count_exp.error());
                }

                problem_dto::testcase_ref testcase_reference_value{
                    .problem_id = problem_reference_value.problem_id,
                    .testcase_order = next_testcase_count_exp->testcase_count
                };
                const auto create_testcase_exp = testcase_mutation_repository::create_testcase(
                    transaction,
                    testcase_reference_value,
                    testcase_value
                );
                if(!create_testcase_exp){
                    return std::unexpected(create_testcase_exp.error());
                }

                testcase_count_value = *next_testcase_count_exp;
            }

            if(has_changes){
                return testcase_mutation_publish_helper::with_published_version(
                    transaction,
                    problem_reference_value,
                    [&](const problem_dto::version& version_value)
                        -> std::expected<problem_dto::testcase_count_mutation_result, service_error> {
                        return testcase_mutation_publish_helper::make_testcase_count_mutation_result(
                            testcase_count_value,
                            problem_reference_value,
                            version_value
                        );
                    }
                );
            }

            return testcase_mutation_publish_helper::with_current_version(
                transaction,
                problem_reference_value,
                [&](const problem_dto::version& version_value)
                    -> std::expected<problem_dto::testcase_count_mutation_result, service_error> {
                    return testcase_mutation_publish_helper::make_testcase_count_mutation_result(
                        testcase_count_value,
                        problem_reference_value,
                        version_value
                    );
                }
            );
        }
    );
}
