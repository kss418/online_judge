#include "db_util/problem_util.hpp"

#include <pqxx/pqxx>

std::expected<bool, error_code> problem_util::exists_problem(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto exists_query_result = transaction.exec(
        "SELECT EXISTS("
        "SELECT 1 "
        "FROM problems "
        "WHERE problem_id = $1"
        ")",
        pqxx::params{problem_id}
    );

    if(exists_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return exists_query_result[0][0].as<bool>();
}

std::expected<void, error_code> problem_util::ensure_statement_row(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "INSERT INTO problem_statements("
        "problem_id, description, input_format, output_format, note, created_at, updated_at"
        ") VALUES($1, '', '', '', NULL, NOW(), NOW()) "
        "ON CONFLICT(problem_id) DO NOTHING",
        pqxx::params{problem_id}
    );

    return {};
}

std::expected<void, error_code> problem_util::increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}

std::expected<std::int32_t, error_code> problem_util::increase_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto ensure_statement_exp = ensure_statement_row(transaction, problem_id);
    if(!ensure_statement_exp){
        return std::unexpected(ensure_statement_exp.error());
    }

    const auto increase_result = transaction.exec(
        "UPDATE problem_statements "
        "SET sample_count = sample_count + 1, updated_at = NOW() "
        "WHERE problem_id = $1 "
        "RETURNING sample_count",
        pqxx::params{problem_id}
    );

    if(increase_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return increase_result[0][0].as<std::int32_t>();
}

std::expected<std::int32_t, error_code> problem_util::decrease_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto decrease_result = transaction.exec(
        "UPDATE problem_statements "
        "SET sample_count = sample_count - 1, updated_at = NOW() "
        "WHERE problem_id = $1 AND sample_count > 0 "
        "RETURNING sample_count",
        pqxx::params{problem_id}
    );

    if(decrease_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    return decrease_result[0][0].as<std::int32_t>();
}

std::expected<std::int64_t, error_code> problem_util::create_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto sample_order_exp = increase_sample_count(transaction, problem_id);
    if(!sample_order_exp){
        return std::unexpected(sample_order_exp.error());
    }

    const std::int32_t sample_order = sample_order_exp.value();
    const auto create_sample_result = transaction.exec(
        "INSERT INTO problem_samples(problem_id, sample_order, sample_input, sample_output) "
        "VALUES($1, $2, $3, $4) "
        "RETURNING sample_id",
        pqxx::params{
            problem_id,
            sample_order,
            sample_value.input,
            sample_value.output
        }
    );

    if(create_sample_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    const auto version_exp = increase_version(transaction, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return create_sample_result[0][0].as<std::int64_t>();
}

std::expected<void, error_code> problem_util::set_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
){
    if(problem_id <= 0 || sample_value.order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "INSERT INTO problem_samples(problem_id, sample_order, sample_input, sample_output) "
        "VALUES($1, $2, $3, $4) "
        "ON CONFLICT(problem_id, sample_order) DO UPDATE "
        "SET "
        "sample_input = EXCLUDED.sample_input, "
        "sample_output = EXCLUDED.sample_output",
        pqxx::params{
            problem_id,
            sample_value.order,
            sample_value.input,
            sample_value.output
        }
    );

    const auto version_exp = increase_version(transaction, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return {};
}

std::expected<void, error_code> problem_util::delete_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
){
    if(problem_id <= 0 || sample_value.order <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto delete_result = transaction.exec(
        "DELETE FROM problem_samples "
        "WHERE "
        "problem_id = $1 AND "
        "sample_order = $2 AND "
        "sample_order = ("
        "SELECT MAX(sample_order) "
        "FROM problem_samples "
        "WHERE problem_id = $1"
        ")",
        pqxx::params{problem_id, sample_value.order}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto sample_count_exp = decrease_sample_count(transaction, problem_id);
    if(!sample_count_exp){
        return std::unexpected(sample_count_exp.error());
    }

    const auto version_exp = increase_version(transaction, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return {};
}
