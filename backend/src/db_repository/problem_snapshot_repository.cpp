#include "db_repository/problem_snapshot_repository.hpp"

#include <pqxx/pqxx>

#include <utility>

namespace{
    struct current_snapshot_manifest{
        std::int32_t version = 0;
        std::int32_t memory_limit_mb = 0;
        std::int32_t time_limit_ms = 0;
        std::int32_t testcase_count = 0;
    };

    std::expected<current_snapshot_manifest, repository_error> load_current_snapshot_manifest(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    ){
        const auto manifest_result = transaction.exec(
            "SELECT "
            "    problem_table.version, "
            "    limits_table.memory_limit_mb, "
            "    limits_table.time_limit_ms, "
            "    COALESCE(testcase_counts.testcase_count, 0) "
            "FROM problems problem_table "
            "JOIN problem_limits limits_table "
            "ON limits_table.problem_id = problem_table.problem_id "
            "LEFT JOIN ("
            "    SELECT "
            "        problem_id, "
            "        COUNT(*)::integer AS testcase_count "
            "    FROM problem_testcases "
            "    WHERE problem_id = $1 "
            "    GROUP BY problem_id"
            ") testcase_counts "
            "ON testcase_counts.problem_id = problem_table.problem_id "
            "WHERE problem_table.problem_id = $1",
            pqxx::params{problem_id}
        );
        if(manifest_result.empty()){
            return std::unexpected(repository_error::not_found);
        }

        current_snapshot_manifest manifest_value;
        manifest_value.version = manifest_result[0][0].as<std::int32_t>();
        manifest_value.memory_limit_mb = manifest_result[0][1].as<std::int32_t>();
        manifest_value.time_limit_ms = manifest_result[0][2].as<std::int32_t>();
        manifest_value.testcase_count = manifest_result[0][3].as<std::int32_t>();
        return manifest_value;
    }

    std::expected<problem_snapshot_repository::manifest, repository_error>
    load_published_snapshot_manifest(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id,
        std::int32_t version
    ){
        const auto manifest_result = transaction.exec(
            "SELECT "
            "    memory_limit_mb, "
            "    time_limit_ms, "
            "    testcase_count "
            "FROM problem_version_manifests "
            "WHERE problem_id = $1 AND version = $2",
            pqxx::params{problem_id, version}
        );
        if(manifest_result.empty()){
            return std::unexpected(repository_error::not_found);
        }

        problem_snapshot_repository::manifest manifest_value;
        manifest_value.problem_id = problem_id;
        manifest_value.version = version;
        manifest_value.limits_value.memory_mb =
            manifest_result[0][0].as<std::int32_t>();
        manifest_value.limits_value.time_ms =
            manifest_result[0][1].as<std::int32_t>();

        const std::int32_t testcase_count = manifest_result[0][2].as<std::int32_t>();
        const auto testcase_result = transaction.exec(
            "SELECT "
            "    testcase_order, "
            "    testcase_input, "
            "    testcase_output "
            "FROM problem_version_testcases "
            "WHERE problem_id = $1 AND version = $2 "
            "ORDER BY testcase_order ASC",
            pqxx::params{problem_id, version}
        );

        manifest_value.testcases.reserve(testcase_result.size());
        for(const auto& testcase_row : testcase_result){
            problem_snapshot_repository::testcase_data testcase_value;
            testcase_value.order = testcase_row[0].as<std::int32_t>();
            testcase_value.input = testcase_row[1].as<std::string>();
            testcase_value.output = testcase_row[2].as<std::string>();
            manifest_value.testcases.push_back(std::move(testcase_value));
        }

        if(static_cast<std::int32_t>(manifest_value.testcases.size()) != testcase_count){
            return std::unexpected(
                repository_error{
                    repository_error_code::internal,
                    "published problem snapshot manifest is inconsistent"
                }
            );
        }

        return manifest_value;
    }
}

std::expected<problem_snapshot_repository::manifest, repository_error>
problem_snapshot_repository::fetch_manifest(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    std::int32_t version
){
    if(problem_id <= 0 || version <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    return load_published_snapshot_manifest(
        transaction,
        problem_id,
        version
    );
}

std::expected<void, repository_error> problem_snapshot_repository::publish_current_snapshot(
    pqxx::transaction_base& transaction,
    const problem_dto::reference& problem_reference_value
){
    if(!problem_dto::is_valid(problem_reference_value)){
        return std::unexpected(repository_error::invalid_reference);
    }

    const std::int64_t problem_id = problem_reference_value.problem_id;
    const auto manifest_exp = load_current_snapshot_manifest(
        transaction,
        problem_id
    );
    if(!manifest_exp){
        return std::unexpected(manifest_exp.error());
    }

    transaction.exec(
        "INSERT INTO problem_version_manifests("
        "    problem_id, "
        "    version, "
        "    memory_limit_mb, "
        "    time_limit_ms, "
        "    testcase_count, "
        "    published_at"
        ") "
        "VALUES($1, $2, $3, $4, $5, NOW()) "
        "ON CONFLICT(problem_id, version) DO UPDATE "
        "SET "
        "    memory_limit_mb = EXCLUDED.memory_limit_mb, "
        "    time_limit_ms = EXCLUDED.time_limit_ms, "
        "    testcase_count = EXCLUDED.testcase_count, "
        "    published_at = NOW()",
        pqxx::params{
            problem_id,
            manifest_exp->version,
            manifest_exp->memory_limit_mb,
            manifest_exp->time_limit_ms,
            manifest_exp->testcase_count
        }
    );

    transaction.exec(
        "DELETE FROM problem_version_testcases "
        "WHERE problem_id = $1 AND version = $2",
        pqxx::params{problem_id, manifest_exp->version}
    );

    transaction.exec(
        "INSERT INTO problem_version_testcases("
        "    problem_id, "
        "    version, "
        "    testcase_order, "
        "    testcase_input, "
        "    testcase_output, "
        "    input_char_count, "
        "    input_line_count, "
        "    output_char_count, "
        "    output_line_count, "
        "    published_at"
        ") "
        "SELECT "
        "    problem_id, "
        "    $2, "
        "    testcase_order, "
        "    testcase_input, "
        "    testcase_output, "
        "    input_char_count, "
        "    input_line_count, "
        "    output_char_count, "
        "    output_line_count, "
        "    NOW() "
        "FROM problem_testcases "
        "WHERE problem_id = $1 "
        "ORDER BY testcase_order ASC",
        pqxx::params{problem_id, manifest_exp->version}
    );

    return {};
}
