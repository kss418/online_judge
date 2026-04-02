#include "db_service/problem_statistics_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_service/service_error_bridge.hpp"
#include "db_repository/problem_statistics_repository.hpp"

std::expected<problem_content_dto::statistics, service_error>
problem_statistics_service::get_statistics(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::map_error_to_service_error(
        db_service_util::with_retry_read_transaction(
            connection,
            [&](pqxx::read_transaction& transaction)
                -> std::expected<problem_content_dto::statistics, error_code> {
                return db_service_error_bridge::map_repository_error(
                    problem_statistics_repository::get_statistics(
                        transaction,
                        problem_reference_value
                    )
                );
            }
        )
    );
}
