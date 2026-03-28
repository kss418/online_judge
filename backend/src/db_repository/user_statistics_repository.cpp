#include "db_repository/user_statistics_repository.hpp"

#include <pqxx/pqxx>

#include <string>
#include <string_view>

namespace{
    std::expected<std::string_view, error_code> get_status_count_column(submission_status status){
        switch(status){
            case submission_status::queued:
                return "queued_submission_count";
            case submission_status::judging:
                return "judging_submission_count";
            case submission_status::accepted:
                return "accepted_submission_count";
            case submission_status::wrong_answer:
                return "wrong_answer_submission_count";
            case submission_status::time_limit_exceeded:
                return "time_limit_exceeded_submission_count";
            case submission_status::memory_limit_exceeded:
                return "memory_limit_exceeded_submission_count";
            case submission_status::runtime_error:
                return "runtime_error_submission_count";
            case submission_status::compile_error:
                return "compile_error_submission_count";
            case submission_status::output_exceeded:
                return "output_exceeded_submission_count";
        }

        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
}

std::expected<user_statistics_dto::submission_statistics, error_code>
user_statistics_repository::get_submission_statistics(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto statistics_query_result = transaction.exec(
        "SELECT "
        "user_id, "
        "submission_count, "
        "queued_submission_count, "
        "judging_submission_count, "
        "accepted_submission_count, "
        "wrong_answer_submission_count, "
        "time_limit_exceeded_submission_count, "
        "memory_limit_exceeded_submission_count, "
        "runtime_error_submission_count, "
        "compile_error_submission_count, "
        "output_exceeded_submission_count, "
        "last_submission_at::text, "
        "last_accepted_at::text, "
        "updated_at::text "
        "FROM user_submission_statistics "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(statistics_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return user_statistics_dto::make_submission_statistics_from_row(
        statistics_query_result[0]
    );
}

std::expected<void, error_code> user_statistics_repository::touch_timestamp_column(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    std::string_view column_name
){
    if(user_id <= 0 || column_name.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string update_query =
        "UPDATE user_submission_statistics "
        "SET " + std::string{column_name} + " = NOW(), "
        "updated_at = NOW() "
        "WHERE user_id = $1";
    const auto update_result = transaction.exec(
        update_query,
        pqxx::params{user_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_statistics_repository::create_user_submission_statistics(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto create_result = transaction.exec(
        "INSERT INTO user_submission_statistics(user_id) "
        "VALUES($1)",
        pqxx::params{user_id}
    );

    if(create_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return {};
}

std::expected<void, error_code> user_statistics_repository::ensure_user_submission_statistics(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "INSERT INTO user_submission_statistics(user_id) "
        "VALUES($1) "
        "ON CONFLICT(user_id) DO NOTHING",
        pqxx::params{user_id}
    );

    return {};
}

std::expected<void, error_code> user_statistics_repository::increase_submission_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto update_result = transaction.exec(
        "UPDATE user_submission_statistics "
        "SET "
        "submission_count = submission_count + 1, "
        "updated_at = NOW() "
        "WHERE user_id = $1",
        pqxx::params{user_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_statistics_repository::increase_status_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    submission_status status
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto column_name_exp = get_status_count_column(status);
    if(!column_name_exp){
        return std::unexpected(column_name_exp.error());
    }

    const std::string update_query =
        "UPDATE user_submission_statistics "
        "SET "
        + std::string{*column_name_exp} + " = " + std::string{*column_name_exp} + " + 1, "
        "updated_at = NOW() "
        "WHERE user_id = $1";
    const auto update_result = transaction.exec(
        update_query,
        pqxx::params{user_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_statistics_repository::decrease_status_count(
    pqxx::transaction_base& transaction,
    std::int64_t user_id,
    submission_status status
){
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto column_name_exp = get_status_count_column(status);
    if(!column_name_exp){
        return std::unexpected(column_name_exp.error());
    }

    const std::string update_query =
        "UPDATE user_submission_statistics "
        "SET "
        + std::string{*column_name_exp} + " = " + std::string{*column_name_exp} + " - 1, "
        "updated_at = NOW() "
        "WHERE user_id = $1 AND " + std::string{*column_name_exp} + " > 0";
    const auto update_result = transaction.exec(
        update_query,
        pqxx::params{user_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<void, error_code> user_statistics_repository::touch_last_submission_at(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    return user_statistics_repository::touch_timestamp_column(
        transaction,
        user_id,
        "last_submission_at"
    );
}

std::expected<void, error_code> user_statistics_repository::touch_last_accepted_at(
    pqxx::transaction_base& transaction,
    std::int64_t user_id
){
    return user_statistics_repository::touch_timestamp_column(
        transaction,
        user_id,
        "last_accepted_at"
    );
}
