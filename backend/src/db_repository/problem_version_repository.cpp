#include "db_repository/problem_version_repository.hpp"

#include <pqxx/pqxx>

std::expected<problem_dto::version, repository_error> problem_version_repository::increase_version(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const std::int64_t problem_id = problem_reference_value.problem_id;
    const auto update_result = transaction.exec(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1 "
        "RETURNING version",
        pqxx::params{problem_id}
    );

    if(update_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    problem_dto::version version_value;
    version_value.version = update_result[0][0].as<std::int32_t>();
    return version_value;
}
