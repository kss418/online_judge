#include "db_util/problem_statistics_util.hpp"

#include <pqxx/pqxx>

std::expected<void, error_code> problem_statistics_util::create_problem_statistics(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto create_result = transaction.exec(
        "INSERT INTO problem_statistics(problem_id) "
        "VALUES($1)",
        pqxx::params{problem_id}
    );
    if(create_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return {};
}

std::expected<void, error_code> problem_statistics_util::increase_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE problem_statistics "
        "SET "
        "submission_count = submission_count + 1, "
        "updated_at = NOW() "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> problem_statistics_util::increase_accepted_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE problem_statistics "
        "SET "
        "accepted_count = accepted_count + 1, "
        "updated_at = NOW() "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
