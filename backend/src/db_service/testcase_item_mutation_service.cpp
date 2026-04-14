#include "db_service/testcase_item_mutation_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_service/testcase_mutation_publish_helper.hpp"

#include "db_repository/problem_content_repository.hpp"
#include "db_repository/testcase_mutation_repository.hpp"
#include "db_repository/testcase_order_repository.hpp"
#include "db_repository/testcase_query_repository.hpp"

std::expected<problem_dto::testcase_mutation_result, service_error>
testcase_item_mutation_service::create_testcase(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::testcase& testcase_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::testcase_mutation_result, service_error> {
            const auto ensure_statement_exp = problem_content_repository::ensure_statement_row(
                transaction,
                problem_reference_value
            );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            const auto testcase_count_exp = testcase_mutation_repository::increase_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!testcase_count_exp){
                return std::unexpected(testcase_count_exp.error());
            }

            problem_dto::testcase_ref testcase_reference_value{
                .problem_id = problem_reference_value.problem_id,
                .testcase_order = testcase_count_exp->testcase_count
            };
            const auto created_testcase_exp = testcase_mutation_repository::create_testcase(
                transaction,
                testcase_reference_value,
                testcase_value
            );
            if(!created_testcase_exp){
                return std::unexpected(created_testcase_exp.error());
            }

            const auto version_exp =
                testcase_mutation_publish_helper::increase_version_and_publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return testcase_mutation_publish_helper::make_testcase_mutation_result(
                *created_testcase_exp,
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_dto::testcase_mutation_result, service_error>
testcase_item_mutation_service::set_testcase_and_get(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::testcase_mutation_result, service_error> {
            const auto set_testcase_exp = testcase_mutation_repository::set_testcase(
                transaction,
                testcase_reference_value,
                testcase_value
            );
            if(!set_testcase_exp){
                return std::unexpected(set_testcase_exp.error());
            }

            const problem_dto::reference problem_reference_value{
                testcase_reference_value.problem_id
            };
            const auto version_exp =
                testcase_mutation_publish_helper::increase_version_and_publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            const auto updated_testcase_exp = testcase_query_repository::get_testcase(
                transaction,
                testcase_reference_value
            );
            if(!updated_testcase_exp){
                return std::unexpected(updated_testcase_exp.error());
            }

            return testcase_mutation_publish_helper::make_testcase_mutation_result(
                *updated_testcase_exp,
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_dto::mutation_result, service_error>
testcase_item_mutation_service::move_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value,
    std::int32_t target_testcase_order
){
    if(target_testcase_order <= 0){
        return std::unexpected(service_error::validation_error);
    }

    const problem_dto::reference problem_reference_value{
        testcase_reference_value.problem_id
    };

    if(testcase_reference_value.testcase_order == target_testcase_order){
        return db_service_util::with_retry_service_read_transaction(
            connection,
            [&](pqxx::read_transaction& transaction)
                -> std::expected<problem_dto::mutation_result, service_error> {
                const auto testcase_exp = testcase_query_repository::get_testcase(
                    transaction,
                    testcase_reference_value
                );
                if(!testcase_exp){
                    return std::unexpected(testcase_exp.error());
                }

                const auto version_exp = testcase_mutation_publish_helper::get_current_version(
                    transaction,
                    problem_reference_value
                );
                if(!version_exp){
                    return std::unexpected(version_exp.error());
                }

                return testcase_mutation_publish_helper::make_mutation_result(
                    problem_reference_value,
                    *version_exp
                );
            }
        );
    }

    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto move_testcase_exp = testcase_order_repository::move_testcase(
                transaction,
                testcase_reference_value,
                target_testcase_order
            );
            if(!move_testcase_exp){
                return std::unexpected(move_testcase_exp.error());
            }

            const auto version_exp =
                testcase_mutation_publish_helper::increase_version_and_publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return testcase_mutation_publish_helper::make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}

std::expected<problem_dto::mutation_result, service_error>
testcase_item_mutation_service::delete_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value
){
    return db_service_util::with_retry_service_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::mutation_result, service_error> {
            const auto delete_testcase_exp =
                testcase_mutation_repository::delete_testcase_and_shift_after(
                    transaction,
                    testcase_reference_value
                );
            if(!delete_testcase_exp){
                return std::unexpected(delete_testcase_exp.error());
            }

            const problem_dto::reference problem_reference_value{
                testcase_reference_value.problem_id
            };
            const auto version_exp =
                testcase_mutation_publish_helper::increase_version_and_publish_current_snapshot(
                    transaction,
                    problem_reference_value
                );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return testcase_mutation_publish_helper::make_mutation_result(
                problem_reference_value,
                *version_exp
            );
        }
    );
}
