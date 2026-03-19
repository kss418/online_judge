#include "db_util/problem_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

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

std::expected<std::int32_t, error_code> problem_util::get_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto version_query_result = transaction.exec(
        "SELECT version "
        "FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(version_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return version_query_result[0][0].as<std::int32_t>();
}

std::expected<std::int64_t, error_code> problem_util::create_problem(
    pqxx::transaction_base& transaction
){
    const auto create_problem_result = transaction.exec(
        "INSERT INTO problems(version) "
        "VALUES($1) "
        "RETURNING problem_id",
        pqxx::params{1}
    );

    if(create_problem_result.empty()){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return create_problem_result[0][0].as<std::int64_t>();
}

std::expected<problem_dto::limits, error_code> problem_util::get_limits(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto limits_query_result = transaction.exec(
        "SELECT memory_limit_mb, time_limit_ms "
        "FROM problem_limits "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(limits_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::limits limits_value;
    limits_value.memory_mb = limits_query_result[0][0].as<std::int32_t>();
    limits_value.time_ms = limits_query_result[0][1].as<std::int32_t>();
    return limits_value;
}

std::expected<void, error_code> problem_util::set_limits(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::limits& limits_value
){
    if(problem_id <= 0 || limits_value.memory_mb <= 0 || limits_value.time_ms <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    transaction.exec(
        "INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at) "
        "VALUES($1, $2, $3, NOW()) "
        "ON CONFLICT(problem_id) DO UPDATE "
        "SET "
        "memory_limit_mb = EXCLUDED.memory_limit_mb, "
        "time_limit_ms = EXCLUDED.time_limit_ms, "
        "updated_at = NOW()",
        pqxx::params{
            problem_id,
            limits_value.memory_mb,
            limits_value.time_ms
        }
    );

    return {};
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

std::expected<problem_dto::statement, error_code> problem_util::get_statement(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto statement_query_result = transaction.exec(
        "SELECT description, input_format, output_format, note "
        "FROM problem_statements "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(statement_query_result.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    problem_dto::statement statement_value;
    statement_value.description = statement_query_result[0][0].as<std::string>();
    statement_value.input_format = statement_query_result[0][1].as<std::string>();
    statement_value.output_format = statement_query_result[0][2].as<std::string>();
    if(!statement_query_result[0][3].is_null()){
        statement_value.note = statement_query_result[0][3].as<std::string>();
    }

    if(
        statement_value.description.empty() &&
        statement_value.input_format.empty() &&
        statement_value.output_format.empty() &&
        !statement_value.note
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return statement_value;
}

std::expected<void, error_code> problem_util::set_statement(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::statement& statement_value
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const std::string note_value = statement_value.note.value_or("");
    transaction.exec(
        "INSERT INTO problem_statements("
        "problem_id, description, input_format, output_format, note, created_at, updated_at"
        ") VALUES($1, $2, $3, $4, NULLIF($5, ''), NOW(), NOW()) "
        "ON CONFLICT(problem_id) DO UPDATE "
        "SET "
        "description = EXCLUDED.description, "
        "input_format = EXCLUDED.input_format, "
        "output_format = EXCLUDED.output_format, "
        "note = NULLIF(EXCLUDED.note, ''), "
        "updated_at = NOW()",
        pqxx::params{
            problem_id,
            statement_value.description,
            statement_value.input_format,
            statement_value.output_format,
            note_value
        }
    );

    return {};
}

std::expected<std::vector<problem_dto::sample>, error_code> problem_util::list_samples(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    const auto samples_query_result = transaction.exec(
        "SELECT sample_id, sample_order, sample_input, sample_output "
        "FROM problem_samples "
        "WHERE problem_id = $1 "
        "ORDER BY sample_order ASC",
        pqxx::params{problem_id}
    );

    std::vector<problem_dto::sample> sample_values;
    sample_values.reserve(samples_query_result.size());
    for(const auto& row : samples_query_result){
        problem_dto::sample sample_value;
        sample_value.id = row[0].as<std::int64_t>();
        sample_value.order = row[1].as<std::int32_t>();
        sample_value.input = row[2].as<std::string>();
        sample_value.output = row[3].as<std::string>();
        sample_values.push_back(std::move(sample_value));
    }

    return sample_values;
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
