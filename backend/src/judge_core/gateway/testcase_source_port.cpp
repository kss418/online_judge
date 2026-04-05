#include "judge_core/gateway/testcase_source_port.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/testcase_service.hpp"

#include <utility>

std::expected<testcase_source_port, judge_error> testcase_source_port::create(
    const db_connection_config& db_config
){
    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return testcase_source_port(std::move(*db_connection_exp));
}

testcase_source_port::testcase_source_port(
    db_connection db_connection_value
) :
    db_connection_(std::move(db_connection_value)){}

testcase_source_port::testcase_source_port(
    testcase_source_port&& other
) noexcept = default;

testcase_source_port& testcase_source_port::operator=(
    testcase_source_port&& other
) noexcept = default;

testcase_source_port::~testcase_source_port() = default;

std::expected<std::int32_t, judge_error> testcase_source_port::fetch_problem_version(
    std::int64_t problem_id
){
    const problem_dto::reference problem_reference_value{problem_id};
    const auto version_exp = problem_core_service::get_version(
        db_connection_,
        problem_reference_value
    );
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    return version_exp->version;
}

std::expected<problem_content_dto::limits, judge_error>
testcase_source_port::fetch_problem_limits(
    std::int64_t problem_id
){
    const problem_dto::reference problem_reference_value{problem_id};
    return problem_content_service::get_limits(
        db_connection_,
        problem_reference_value
    );
}

std::expected<std::int32_t, judge_error> testcase_source_port::fetch_testcase_count(
    std::int64_t problem_id
){
    const problem_dto::reference problem_reference_value{problem_id};
    const auto testcase_count_exp = testcase_service::get_testcase_count(
        db_connection_,
        problem_reference_value
    );
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    return testcase_count_exp->testcase_count;
}

std::expected<testcase_source_port::testcase_data, judge_error>
testcase_source_port::fetch_testcase(
    std::int64_t problem_id,
    std::int32_t order
){
    const problem_dto::testcase_ref testcase_reference_value{
        .problem_id = problem_id,
        .testcase_order = order
    };
    const auto testcase_exp = testcase_service::get_testcase(
        db_connection_,
        testcase_reference_value
    );
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    testcase_data testcase_data_value;
    testcase_data_value.order = testcase_exp->order;
    testcase_data_value.input = std::move(testcase_exp->input);
    testcase_data_value.output = std::move(testcase_exp->output);
    return testcase_data_value;
}
