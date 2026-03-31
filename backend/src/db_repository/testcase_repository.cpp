#include "db_repository/testcase_repository.hpp"

#include <pqxx/pqxx>
#include <utility>

std::expected<problem_dto::testcase, error_code> testcase_repository::create_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::testcase created_testcase_value = testcase_value;
    created_testcase_value.id = create_testcase_result[0][0].as<std::int64_t>();
    created_testcase_value.order = testcase_order;
    return created_testcase_value;
}

std::expected<problem_dto::testcase, error_code> testcase_repository::get_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcase_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{problem_id, testcase_order}
    );

    if(testcase_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase testcase_value;
    testcase_value.id = testcase_query_result[0][0].as<std::int64_t>();
    testcase_value.order = testcase_query_result[0][1].as<std::int32_t>();
    testcase_value.input = testcase_query_result[0][2].as<std::string>();
    testcase_value.output = testcase_query_result[0][3].as<std::string>();
    return testcase_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::get_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcase_count_query_result = transaction.exec(
        "SELECT COUNT(*) "
        "FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(testcase_count_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = testcase_count_query_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::increase_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto increase_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = increase_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::decrease_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = testcase_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND testcase_count > 0 "
        "RETURNING testcase_count",
        pqxx::params{problem_reference_value.problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = decrease_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}

std::expected<std::vector<problem_dto::testcase>, error_code> testcase_repository::list_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcases_query_result = transaction.exec(
        "SELECT testcase_id, testcase_order, testcase_input, testcase_output "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::testcase> testcase_values;
    testcase_values.reserve(testcases_query_result.size());
    for(const auto& row : testcases_query_result){
        problem_dto::testcase testcase_value;
        testcase_value.id = row[0].as<std::int64_t>();
        testcase_value.order = row[1].as<std::int32_t>();
        testcase_value.input = row[2].as<std::string>();
        testcase_value.output = row[3].as<std::string>();
        testcase_values.push_back(std::move(testcase_value));
    }

    return testcase_values;
}

std::expected<std::vector<problem_dto::testcase_summary>, error_code>
testcase_repository::list_testcase_summaries(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto testcases_query_result = transaction.exec(
        "SELECT "
        "testcase_id, "
        "testcase_order, "
        "input_char_count, "
        "input_line_count, "
        "output_char_count, "
        "output_line_count "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::testcase_summary> testcase_summary_values;
    testcase_summary_values.reserve(testcases_query_result.size());
    for(const auto& row : testcases_query_result){
        problem_dto::testcase_summary testcase_summary_value;
        testcase_summary_value.id = row[0].as<std::int64_t>();
        testcase_summary_value.order = row[1].as<std::int32_t>();
        testcase_summary_value.input_char_count = row[2].as<std::int32_t>();
        testcase_summary_value.input_line_count = row[3].as<std::int32_t>();
        testcase_summary_value.output_char_count = row[4].as<std::int32_t>();
        testcase_summary_value.output_line_count = row[5].as<std::int32_t>();
        testcase_summary_values.push_back(std::move(testcase_summary_value));
    }

    return testcase_summary_values;
}

std::expected<void, error_code> testcase_repository::set_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    const problem_dto::testcase& testcase_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> testcase_repository::move_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value,
    std::int32_t target_testcase_order
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t source_testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || source_testcase_order <= 0 || target_testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    if(source_testcase_order == target_testcase_order){
        const auto testcase_exp = get_testcase(transaction, testcase_reference_value);
        if(!testcase_exp){
            return std::unexpected(testcase_exp.error());
        }

        return {};
    }

    transaction.exec(
        "SET CONSTRAINTS problem_testcases_problem_id_testcase_order_unique DEFERRED"
    );

    const std::int32_t min_testcase_order = std::min(
        source_testcase_order,
        target_testcase_order
    );
    const std::int32_t max_testcase_order = std::max(
        source_testcase_order,
        target_testcase_order
    );

    const auto move_result = transaction.exec(
        "UPDATE problem_testcases "
        "SET testcase_order = CASE "
        "WHEN testcase_order = $2 THEN $3 "
        "WHEN $2 < $3 AND testcase_order > $2 AND testcase_order <= $3 "
        "THEN testcase_order - 1 "
        "WHEN $2 > $3 AND testcase_order >= $3 AND testcase_order < $2 "
        "THEN testcase_order + 1 "
        "ELSE testcase_order "
        "END "
        "WHERE "
        "problem_id = $1 AND "
        "testcase_order BETWEEN $4 AND $5",
        pqxx::params{
            problem_id,
            source_testcase_order,
            target_testcase_order,
            min_testcase_order,
            max_testcase_order
        }
    );

    const auto expected_affected_rows = static_cast<decltype(move_result.affected_rows())>(
        max_testcase_order - min_testcase_order + 1
    );
    if(move_result.affected_rows() != expected_affected_rows){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> testcase_repository::delete_testcase(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto delete_result = transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE problem_id = $1 AND testcase_order = $2",
        pqxx::params{problem_id, testcase_order}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> testcase_repository::decrease_order(
    pqxx::transaction_base& transaction,
    const problem_dto::testcase_ref& testcase_reference_value
){
    const std::int64_t problem_id = testcase_reference_value.problem_id;
    const std::int32_t testcase_order = testcase_reference_value.testcase_order;
    if(problem_id <= 0 || testcase_order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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

std::expected<void, error_code> testcase_repository::delete_testcase_and_shift_after(
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

std::expected<void, error_code> testcase_repository::delete_all_testcases(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "DELETE FROM problem_testcases "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    return {};
}

std::expected<problem_dto::testcase_count, error_code> testcase_repository::clear_testcase_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const std::int64_t problem_id = problem_reference_value.problem_id;
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto clear_result = transaction.exec(
        "UPDATE problem_statements "
        "SET testcase_count = 0, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING testcase_count",
        pqxx::params{problem_id}
    );

    if(clear_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::testcase_count testcase_count_value;
    testcase_count_value.testcase_count = clear_result[0][0].as<std::int32_t>();
    return testcase_count_value;
}
