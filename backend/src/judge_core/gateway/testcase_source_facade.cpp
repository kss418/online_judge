#include "judge_core/gateway/testcase_source_facade.hpp"

#include "db_repository/problem_snapshot_repository.hpp"
#include "db_service/db_service_util.hpp"
#include "error/service_error.hpp"

#include <utility>

std::expected<testcase_source_facade, judge_error> testcase_source_facade::create(
    const db_connection_config& db_config
){
    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return testcase_source_facade(std::move(*db_connection_exp));
}

testcase_source_facade::testcase_source_facade(
    db_connection db_connection_value
) :
    db_connection_(std::move(db_connection_value)){}

testcase_source_facade::testcase_source_facade(
    testcase_source_facade&& other
) noexcept = default;

testcase_source_facade& testcase_source_facade::operator=(
    testcase_source_facade&& other
) noexcept = default;

testcase_source_facade::~testcase_source_facade() = default;

std::expected<testcase_source_facade::problem_snapshot_manifest, judge_error>
testcase_source_facade::fetch_manifest(
    std::int64_t problem_id,
    std::int32_t version
){
    const auto manifest_exp = db_service_util::with_retry_service_read_transaction(
        db_connection_,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<problem_snapshot_manifest, service_error> {
            const auto repository_manifest_exp =
                problem_snapshot_repository::fetch_manifest(
                    transaction,
                    problem_id,
                    version
                );
            if(!repository_manifest_exp){
                return std::unexpected(service_error{repository_manifest_exp.error()});
            }

            problem_snapshot_manifest manifest_value;
            manifest_value.problem_id = repository_manifest_exp->problem_id;
            manifest_value.version = repository_manifest_exp->version;
            manifest_value.limits_value = repository_manifest_exp->limits_value;
            manifest_value.testcases.reserve(
                repository_manifest_exp->testcases.size()
            );
            for(auto& testcase_value : repository_manifest_exp->testcases){
                testcase_data facade_testcase_value;
                facade_testcase_value.order = testcase_value.order;
                facade_testcase_value.input = std::move(testcase_value.input);
                facade_testcase_value.output = std::move(testcase_value.output);
                manifest_value.testcases.push_back(std::move(facade_testcase_value));
            }

            return manifest_value;
        }
    );
    if(!manifest_exp){
        return std::unexpected(judge_error{manifest_exp.error()});
    }

    return std::move(*manifest_exp);
}
