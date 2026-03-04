#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <cstdint>
#include <expected>
#include <string>

class problem_content_service : public db_service_base<problem_content_service>{
public:
    std::expected<void, error_code> set_statement(
        std::int64_t problem_id,
        const std::string& description,
        const std::string& input_format,
        const std::string& output_format,
        const std::string& note
    );

    std::expected<std::int64_t, error_code> create_sample(
        std::int64_t problem_id,
        const std::string& sample_input,
        const std::string& sample_output
    );

    std::expected<void, error_code> set_sample(
        std::int64_t problem_id,
        std::int32_t sample_order,
        const std::string& sample_input,
        const std::string& sample_output
    );

    std::expected<void, error_code> delete_sample(
        std::int64_t problem_id,
        std::int32_t sample_order
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

    std::expected<std::int32_t, error_code> increase_testcase_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> decrease_testcase_count(
        pqxx::work& transaction,
        std::int64_t problem_id
    );

    friend class db_service_base<problem_content_service>;

    explicit problem_content_service(db_connection connection);
};
