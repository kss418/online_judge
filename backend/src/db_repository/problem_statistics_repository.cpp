#include "db_repository/problem_statistics_repository.hpp"
#include "error/repository_error.hpp"

#include <pqxx/pqxx>

std::expected<problem_content_dto::statistics, repository_error>
problem_statistics_repository::get_statistics(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto statistics_query_result = transaction.exec(
        "SELECT submission_count, accepted_count "
        "FROM problem_statistics "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(statistics_query_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_content_dto::statistics statistics_value;
    statistics_value.submission_count = statistics_query_result[0][0].as<std::int64_t>();
    statistics_value.accepted_count = statistics_query_result[0][1].as<std::int64_t>();
    return statistics_value;
}

std::expected<void, repository_error> problem_statistics_repository::create_problem_statistics(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto create_result = transaction.exec(
        "INSERT INTO problem_statistics(problem_id) "
        "VALUES($1)",
        pqxx::params{problem_id}
    );
    if(create_result.affected_rows() == 0){
        return std::unexpected(repository_error::internal);
    }

    return {};
}

std::expected<void, repository_error> problem_statistics_repository::increase_submission_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto update_result = transaction.exec(
        "UPDATE problem_statistics "
        "SET "
        "submission_count = submission_count + 1, "
        "updated_at = NOW() "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> problem_statistics_repository::increase_accepted_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto update_result = transaction.exec(
        "UPDATE problem_statistics "
        "SET "
        "accepted_count = accepted_count + 1, "
        "updated_at = NOW() "
        "WHERE problem_id = $1",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    return {};
}

std::expected<void, repository_error> problem_statistics_repository::decrease_accepted_count(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }
    const std::int64_t problem_id = problem_reference_value.problem_id;

    const auto update_result = transaction.exec(
        "UPDATE problem_statistics "
        "SET "
        "accepted_count = accepted_count - 1, "
        "updated_at = NOW() "
        "WHERE problem_id = $1 AND accepted_count > 0",
        pqxx::params{problem_id}
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(repository_error::conflict);
    }

    return {};
}
