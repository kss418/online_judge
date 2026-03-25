#include "db_service/problem_statistics_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_util/problem_statistics_util.hpp"

std::expected<problem_content_dto::statistics, error_code>
problem_statistics_service::get_statistics(
    db_connection& connection,
    const problem_dto::reference& problem_reference_value
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_content_dto::statistics, error_code> {
            return problem_statistics_util::get_statistics(
                transaction,
                problem_reference_value
            );
        }
    );
}
