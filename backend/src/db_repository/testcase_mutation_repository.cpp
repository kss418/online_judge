#include "db_repository/testcase_mutation_repository.hpp"

#include <pqxx/pqxx>

namespace{
    std::expected<void, repository_error> decrease_order(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value
    ){
        const std::int64_t problem_id = testcase_reference_value.problem_id;
        const std::int32_t testcase_order = testcase_reference_value.testcase_order;
        if(!problem_dto::is_valid(testcase_reference_value)){
            return std::unexpected(repository_error::invalid_reference);
        }

        transaction.exec(
            "SET CONSTRAINTS problem_testcases_problem_id_testcase_order_unique DEFERRED"
        );

        transaction.exec(
            "UPDATE problem_testcases "
            "SET testcase_order = testcase_order - 1 "
            "WHERE problem_id = $1 AND testcase_order > $2",
            pqxx::params{problem_id, testcase_order}
        );

        return {};
    }
}

std::expected<problem_dto::testcase, repository_error> testcase_mutation_repository::create_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(!problem_dto::is_valid(testcase_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const auto create_testcase_result = transaction.exec(
        "INSERT INTO problem_testcases("
        "problem_id, testcase_order, testcase_input, testcase_output, "
        "input_char_count, input_line_count, output_char_count, output_line_count"
        ") "
        "VALUES("
        "$1, $2, $3, $4, "
        "char_length($3), "
        "CASE WHEN $3 = '' THEN 0 "
        "ELSE 1 + char_length($3) - char_length(replace($3, E'\\n', '')) END, "
        "char_length($4), "
        "CASE WHEN $4 = '' THEN 0 "
        "ELSE 1 + char_length($4) - char_length(replace($4, E'\\n', '')) END"
        ") "
        "RETURNING testcase_id",
        pqxx::params{
            problem_id,
            testcase_order,
            testcase_value.input,
            testcase_value.output
        }
    );

    if(create_testcase_result.empty()){
        return std::unexpected(repository_error::internal);
    }

    problem_dto::testcase created_testcase_value = testcase_value;
    created_testcase_value.id = create_testcase_result[0][0].as<std::int64_t>();
    created_testcase_value.order = testcase_order;
    return created_testcase_value;
}

std::expected<problem_dto::testcase_count, repository_error>
testcase_mutation_repository::increase_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto increase_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(increase_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = increase_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<problem_dto::testcase_count, repository_error>
testcase_mutation_repository::decrease_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND testcase_count > 0 "
        "RETURNING testcase_count",
        pqxx::params{problem_reference_value.problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(repository_error::conflict);
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = decrease_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<void, repository_error> testcase_mutation_repository::set_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(!problem_dto::is_valid(testcase_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto update_result = transaction.exec(
        "UPDATE problem_testcases "
        "SET "
        "testcase_input = $3, "
        "testcase_output = $4, "
        "input_char_count = char_length($3), "
        "input_line_count = CASE WHEN $3 = '' THEN 0 "
        "ELSE 1 + char_length($3) - char_length(replace($3, E'\\n', '')) END, "
        "output_char_count = char_length($4), "
        "output_line_count = CASE WHEN $4 = '' THEN 0 "
        "ELSE 1 + char_length($4) - char_length(replace($4, E'\\n', '')) END "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{
            problem_id,
            testcase_order,
            testcase_value.input,
            testcase_value.output
        }
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> testcase_mutation_repository::delete_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(!problem_dto::is_valid(testcase_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto delete_result = transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{problem_id, testcase_order}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> testcase_mutation_repository::delete_testcase_and_shift_after(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const auto delete_testcase_exp = delete_testcase(
        transaction,
        testcase_reference_value
    );
    if(!delete_testcase_exp){
        return std::unexpected(delete_testcase_exp.error());
    }

    const auto decrease_order_exp = decrease_order(
        transaction,
        testcase_reference_value
    );
    if(!decrease_order_exp){
        return std::unexpected(decrease_order_exp.error());
    }

    problem_dto::reference problem_reference_value{
        testcase_reference_value.problem_id
    };
    const auto decreased_testcase_count_exp = decrease_testcase_count(
        transaction,
        problem_reference_value
    );
    if(!decreased_testcase_count_exp){
        return std::unexpected(decreased_testcase_count_exp.error());
    }

    return {};
}

std::expected<void, repository_error> testcase_mutation_repository::delete_all_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    return {};
}

std::expected<problem_dto::testcase_count, repository_error>
testcase_mutation_repository::clear_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto clear_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = 0, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(clear_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = clear_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}
