#include "db_service/testcase_service.hpp"
#include "db_service/db_service_util.hpp"

#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"
#include "db_util/testcase_util.hpp"

std::expected<problem_dto::testcase, error_code> testcase_service::create_testcase(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::testcase& testcase_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::testcase, error_code> {
            const auto ensure_statement_exp = problem_content_util::ensure_statement_row(
                transaction,
                problem_reference_value
            );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            const auto testcase_count_exp = testcase_util::increase_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!testcase_count_exp){
                return std::unexpected(testcase_count_exp.error());
            }
            problem_dto::testcase_ref testcase_reference_value;
            testcase_reference_value.problem_id = problem_reference_value.problem_id;
            testcase_reference_value.testcase_order = testcase_count_exp->testcase_count;

            const auto created_testcase_exp = testcase_util::create_testcase(
                transaction,
                testcase_reference_value,
                testcase_value
            );
            if(!created_testcase_exp){
                return std::unexpected(created_testcase_exp.error());
            }

            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return *created_testcase_exp;
        }
    );
}

std::expected<problem_dto::testcase, error_code> testcase_service::get_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value
){
    if(
        testcase_reference_value.problem_id <= 0 ||
        testcase_reference_value.testcase_order <= 0
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::testcase, error_code> {
            return testcase_util::get_testcase(
                transaction,
                testcase_reference_value
            );
        }
    );
}

std::expected<problem_dto::testcase_count, error_code> testcase_service::get_testcase_count(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_dto::testcase_count, error_code> {
            return testcase_util::get_testcase_count(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<std::vector<problem_dto::testcase>, error_code> testcase_service::list_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<problem_dto::testcase>, error_code> {
            return testcase_util::list_testcases(
                transaction,
                problem_reference_value
            );
        }
    );
}

std::expected<void, error_code> testcase_service::set_testcase(
    db_connection& connection,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    if(
        testcase_reference_value.problem_id <= 0 ||
        testcase_reference_value.testcase_order <= 0
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto set_testcase_exp = testcase_util::set_testcase(
                transaction,
                testcase_reference_value,
                testcase_value
            );
            if(!set_testcase_exp){
                return std::unexpected(set_testcase_exp.error());
            }

            problem_dto::reference problem_reference_value{
                testcase_reference_value.problem_id
            };
            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<void, error_code> testcase_service::delete_testcase(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto delete_testcase_exp = testcase_util::delete_testcase(
                transaction,
                problem_reference_value
            );
            if(!delete_testcase_exp){
                return std::unexpected(delete_testcase_exp.error());
            }

            const auto testcase_count_exp = testcase_util::decrease_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!testcase_count_exp){
                return std::unexpected(testcase_count_exp.error());
            }

            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<void, error_code> testcase_service::delete_all_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    if(problem_reference_value.problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto testcase_count_exp = testcase_util::get_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!testcase_count_exp){
                return std::unexpected(testcase_count_exp.error());
            }

            if(testcase_count_exp->testcase_count <= 0){
                return {};
            }

            const auto delete_all_testcases_exp = testcase_util::delete_all_testcases(
                transaction,
                problem_reference_value
            );
            if(!delete_all_testcases_exp){
                return std::unexpected(delete_all_testcases_exp.error());
            }

            const auto clear_testcase_count_exp = testcase_util::clear_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!clear_testcase_count_exp){
                return std::unexpected(clear_testcase_count_exp.error());
            }

            const auto version_exp = problem_core_util::increase_version(
                transaction,
                problem_reference_value
            );
            if(!version_exp){
                return std::unexpected(version_exp.error());
            }

            return {};
        }
    );
}

std::expected<problem_dto::testcase_count, error_code> testcase_service::replace_testcases(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value,
    const std::vector<problem_dto::testcase>& testcase_values
){
    if(
        problem_reference_value.problem_id <= 0 ||
        testcase_values.size() > 999
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<problem_dto::testcase_count, error_code> {
            const auto ensure_statement_exp = problem_content_util::ensure_statement_row(
                transaction,
                problem_reference_value
            );
            if(!ensure_statement_exp){
                return std::unexpected(ensure_statement_exp.error());
            }

            const auto current_testcase_count_exp = testcase_util::get_testcase_count(
                transaction,
                problem_reference_value
            );
            if(!current_testcase_count_exp){
                return std::unexpected(current_testcase_count_exp.error());
            }

            if(current_testcase_count_exp->testcase_count > 0){
                const auto delete_all_testcases_exp = testcase_util::delete_all_testcases(
                    transaction,
                    problem_reference_value
                );
                if(!delete_all_testcases_exp){
                    return std::unexpected(delete_all_testcases_exp.error());
                }

                const auto clear_testcase_count_exp = testcase_util::clear_testcase_count(
                    transaction,
                    problem_reference_value
                );
                if(!clear_testcase_count_exp){
                    return std::unexpected(clear_testcase_count_exp.error());
                }
            }

            problem_dto::testcase_count testcase_count_value{
                .testcase_count = 0
            };
            for(const auto& testcase_value : testcase_values){
                const auto next_testcase_count_exp = testcase_util::increase_testcase_count(
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
                const auto create_testcase_exp = testcase_util::create_testcase(
                    transaction,
                    testcase_reference_value,
                    testcase_value
                );
                if(!create_testcase_exp){
                    return std::unexpected(create_testcase_exp.error());
                }

                testcase_count_value = *next_testcase_count_exp;
            }

            if(
                current_testcase_count_exp->testcase_count > 0 ||
                !testcase_values.empty()
            ){
                const auto version_exp = problem_core_util::increase_version(
                    transaction,
                    problem_reference_value
                );
                if(!version_exp){
                    return std::unexpected(version_exp.error());
                }
            }

            return testcase_count_value;
        }
    );
}
