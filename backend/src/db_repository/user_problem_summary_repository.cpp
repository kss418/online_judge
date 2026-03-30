#include "db_repository/user_problem_summary_repository.hpp"

#include <pqxx/pqxx>

namespace{
    std::expected<void, error_code> validate_summary_counts(
        std::int64_t user_id,
        std::int64_t problem_id,
        std::int64_t submission_count,
        std::int64_t accepted_submission_count,
        std::int64_t failed_submission_count
    ){
        if(
            user_id <= 0 ||
            problem_id <= 0 ||
            submission_count <= 0 ||
            accepted_submission_count < 0 ||
            failed_submission_count < 0 ||
            accepted_submission_count + failed_submission_count > submission_count
        ){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return {};
    }

    std::expected<void, error_code> validate_summary_key(
        std::int64_t user_id,
        std::int64_t problem_id
    ){
        if(user_id <= 0 || problem_id <= 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        return {};
    }
}

std::expected<void, error_code> user_problem_summary_repository::upsert_summary_counts(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id,
    std::int64_t submission_count,
    std::int64_t accepted_submission_count,
    std::int64_t failed_submission_count
){
    const auto validate_exp = validate_summary_counts(
        user_id,
        problem_id,
        submission_count,
        accepted_submission_count,
        failed_submission_count
    );
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    transaction.exec(
        "INSERT INTO user_problem_attempt_summary("
        "user_id, "
        "problem_id, "
        "submission_count, "
        "accepted_submission_count, "
        "failed_submission_count, "
        "updated_at"
        ") "
        "VALUES($1, $2, $3, $4, $5, NOW()) "
        "ON CONFLICT(user_id, problem_id) DO UPDATE "
        "SET "
        "submission_count = EXCLUDED.submission_count, "
        "accepted_submission_count = EXCLUDED.accepted_submission_count, "
        "failed_submission_count = EXCLUDED.failed_submission_count, "
        "updated_at = NOW()",
        pqxx::params{
            user_id,
            problem_id,
            submission_count,
            accepted_submission_count,
            failed_submission_count
        }
    );

    return {};
}

std::expected<void, error_code> user_problem_summary_repository::increase_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id
){
    const auto validate_exp = validate_summary_key(user_id, problem_id);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    transaction.exec(
        "INSERT INTO user_problem_attempt_summary("
        "user_id, "
        "problem_id, "
        "submission_count, "
        "accepted_submission_count, "
        "failed_submission_count, "
        "updated_at"
        ") "
        "VALUES($1, $2, 1, 0, 0, NOW()) "
        "ON CONFLICT(user_id, problem_id) DO UPDATE "
        "SET "
        "submission_count = user_problem_attempt_summary.submission_count + 1, "
        "updated_at = NOW()",
        pqxx::params{user_id, problem_id}
    );

    return {};
}

std::expected<void, error_code> user_problem_summary_repository::increase_accepted_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id
){
    const auto validate_exp = validate_summary_key(user_id, problem_id);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    const auto update_result = transaction.exec(
        "UPDATE user_problem_attempt_summary "
        "SET "
        "accepted_submission_count = accepted_submission_count + 1, "
        "updated_at = NOW() "
        "WHERE "
        "user_id = $1 AND "
        "problem_id = $2 AND "
        "accepted_submission_count + failed_submission_count < submission_count",
        pqxx::params{user_id, problem_id}
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_problem_summary_repository::decrease_accepted_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id
){
    const auto validate_exp = validate_summary_key(user_id, problem_id);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    const auto update_result = transaction.exec(
        "UPDATE user_problem_attempt_summary "
        "SET "
        "accepted_submission_count = accepted_submission_count - 1, "
        "updated_at = NOW() "
        "WHERE "
        "user_id = $1 AND "
        "problem_id = $2 AND "
        "accepted_submission_count > 0",
        pqxx::params{user_id, problem_id}
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_problem_summary_repository::increase_failed_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id
){
    const auto validate_exp = validate_summary_key(user_id, problem_id);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    const auto update_result = transaction.exec(
        "UPDATE user_problem_attempt_summary "
        "SET "
        "failed_submission_count = failed_submission_count + 1, "
        "updated_at = NOW() "
        "WHERE "
        "user_id = $1 AND "
        "problem_id = $2 AND "
        "accepted_submission_count + failed_submission_count < submission_count",
        pqxx::params{user_id, problem_id}
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_problem_summary_repository::decrease_failed_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::int64_t problem_id
){
    const auto validate_exp = validate_summary_key(user_id, problem_id);
    if(!validate_exp){
        return std::unexpected(validate_exp.error());
    }

    const auto update_result = transaction.exec(
        "UPDATE user_problem_attempt_summary "
        "SET "
        "failed_submission_count = failed_submission_count - 1, "
        "updated_at = NOW() "
        "WHERE "
        "user_id = $1 AND "
        "problem_id = $2 AND "
        "failed_submission_count > 0",
        pqxx::params{user_id, problem_id}
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
