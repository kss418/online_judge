#include "db_service/problem_version_publish_service.hpp"

#include "db_repository/problem_core_repository.hpp"
#include "db_repository/problem_snapshot_repository.hpp"

std::expected<void, service_error> problem_version_publish_service::publish_current_snapshot(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const auto publish_snapshot_exp =
        problem_snapshot_repository::publish_current_snapshot(
            transaction,
            problem_reference_value
        );
    if(!publish_snapshot_exp){
        return std::unexpected(publish_snapshot_exp.error());
    }

    return {};
}

std::expected<problem_dto::version, service_error>
problem_version_publish_service::increase_version_and_publish_current_snapshot(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    const auto version_exp = problem_core_repository::increase_version(
        transaction,
        problem_reference_value
    );
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto publish_snapshot_exp = publish_current_snapshot(
        transaction,
        problem_reference_value
    );
    if(!publish_snapshot_exp){
        return std::unexpected(publish_snapshot_exp.error());
    }

    return *version_exp;
}
