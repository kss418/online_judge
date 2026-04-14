#include "db_repository/problem_mutation_repository.hpp"

#include <pqxx/pqxx>

std::expected<problem_dto::created, repository_error> problem_mutation_repository::create_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::create_request& create_request_value
){
    if(!problem_dto::is_valid(create_request_value)){
        return std::unexpected(repository_error::invalid_input);
    }

    const auto create_problem_result = transaction.exec(
        "INSERT INTO problems(version, title) "
        "VALUES($1, $2) "
        "RETURNING problem_id",
        pqxx::params{1, create_request_value.title}
    );

    if(create_problem_result.empty()){
        return std::unexpected(repository_error::internal);
    }

    problem_dto::created created_value;
    created_value.problem_id = create_problem_result[0][0].as<std::int64_t>();
    return created_value;
}

std::expected<void, repository_error> problem_mutation_repository::set_title(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_dto::title& title_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    if(!problem_dto::is_valid(title_value)){
        return std::unexpected(repository_error::invalid_input);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET title = $2 "
        "WHERE problem_id = $1",
        pqxx::params{problem_id, title_value.value}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> problem_mutation_repository::delete_problem(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto delete_result = transaction.exec(
        "DELETE FROM problems "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(delete_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> problem_mutation_repository::set_limits(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value,
    const problem_content_dto::limits& limits_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    if(!problem_content_dto::is_valid(limits_value)){
        return std::unexpected(repository_error::invalid_input);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

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
