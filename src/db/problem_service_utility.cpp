#include "db/problem_service_utility.hpp"

#include <pqxx/pqxx>

namespace problem_service_utility{
std::expected<void, error_code> increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
){
    const auto update_result = transaction.exec_params(
        "UPDATE problems "
        "SET version = version + 1 "
        "WHERE problem_id = $1",
        problem_id
    );

    if(update_result.affected_rows() == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return {};
}
}
