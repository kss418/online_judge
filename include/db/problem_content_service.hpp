#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"
#include "dto/problem_dto.hpp"

#include <expected>
#include <string>
#include <vector>

class problem_content_service : public db_service_base<problem_content_service>{
public:
    std::expected<problem_statement, error_code> get_statement(std::int64_t problem_id);

    std::expected<void, error_code> set_statement(
        std::int64_t problem_id,
        const problem_statement& statement
    );

    std::expected<std::vector<sample>, error_code> list_samples(std::int64_t problem_id);

    std::expected<std::int64_t, error_code> create_sample(
        std::int64_t problem_id,
        const sample& sample_value
    );

    std::expected<void, error_code> set_sample(
        std::int64_t problem_id,
        const sample& sample_value
    );

    std::expected<void, error_code> delete_sample(
        std::int64_t problem_id,
        const sample& sample_value
    );

private:
    std::expected<std::int32_t, error_code> increase_sample_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> decrease_sample_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    friend class db_service_base<problem_content_service>;

    explicit problem_content_service(db_connection connection);
};
