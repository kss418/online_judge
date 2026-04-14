#include "db_repository/submission_queue_repository.hpp"

#include "common/submission_status.hpp"
#include "row_mapper/submission_row_mapper.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<submission_response_dto::queued_response, repository_error>
submission_queue_repository::create_submission(
    pqxx::transaction_base& transaction,
    const submission_internal_dto::create_submission_command& create_request_value
){
    if(!submission_internal_dto::is_valid(create_request_value)){
        return std::unexpected(repository_error::invalid_input);
    }

    const auto create_submission_result = transaction.exec(
        "INSERT INTO submissions(user_id, problem_id, problem_version, language, source_code) "
        "SELECT "
        "    $1, "
        "    problem_table.problem_id, "
        "    problem_table.version, "
        "    $3, "
        "    $4 "
        "FROM problems problem_table "
        "WHERE problem_table.problem_id = $2 "
        "RETURNING submission_id, problem_version",
        pqxx::params{
            create_request_value.user_id,
            create_request_value.problem_id,
            create_request_value.source_value.language,
            create_request_value.source_value.source_code
        }
    );

    if(create_submission_result.empty()){
        return std::unexpected(repository_error::not_found);
    }

    const std::int64_t submission_id = create_submission_result[0][0].as<std::int64_t>();
    const std::int32_t problem_version = create_submission_result[0][1].as<std::int32_t>();
    transaction.exec(
        "INSERT INTO submission_status_history(submission_id, from_status, to_status, reason) "
        "VALUES($1, NULL, $2::submission_status, NULL)",
        pqxx::params{submission_id, to_string(submission_status::queued)}
    );

    return submission_response_dto::make_queued_response(
        submission_id,
        submission_status::queued,
        problem_version
    );
}

std::expected<void, repository_error> submission_queue_repository::enqueue_submission(
    pqxx::transaction_base& transaction,
    std::int64_t submission_id,
    std::int16_t priority
){
    if(submission_id <= 0){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto enqueue_result = transaction.exec(
        "INSERT INTO submission_queue(submission_id, priority) "
        "SELECT "
        "submission_table.submission_id, "
        "$2 "
        "FROM submissions submission_table "
        "WHERE submission_table.submission_id = $1",
        pqxx::params{submission_id, priority}
    );
    if(enqueue_result.affected_rows() == 0){
        return std::unexpected(repository_error::not_found);
    }

    transaction.exec(
        "SELECT pg_notify($1, $2)",
        pqxx::params{SUBMISSION_QUEUE_CHANNEL, std::to_string(submission_id)}
    );

    return {};
}

std::expected<std::optional<submission_domain_dto::leased_submission>, repository_error>
submission_queue_repository::lease_submission(
    pqxx::transaction_base& transaction,
    const submission_internal_dto::lease_request& lease_request_value
){
    const std::chrono::seconds lease_duration = lease_request_value.lease_duration;
    if(!submission_internal_dto::is_valid(lease_request_value)){
        return std::unexpected(repository_error::invalid_input);
    }

    const auto leased_submission_result = transaction.exec(
        "WITH lease_candidate AS ("
        "    SELECT "
        "        queue_table.submission_id, "
        "        submission_table.problem_id, "
        "        submission_table.problem_version, "
        "        GREATEST(0::bigint, FLOOR(EXTRACT(EPOCH FROM (NOW() - queue_table.available_at)) * 1000))::bigint AS queue_wait_ms, "
        "        submission_table.language, "
        "        submission_table.source_code "
        "    FROM submission_queue queue_table "
        "    JOIN submissions submission_table "
        "    ON submission_table.submission_id = queue_table.submission_id "
        "    WHERE "
        "        queue_table.available_at <= NOW() AND "
        "        (queue_table.leased_until IS NULL OR queue_table.leased_until <= NOW()) "
        "    ORDER BY queue_table.priority DESC, queue_table.created_at ASC "
        "    FOR UPDATE OF queue_table SKIP LOCKED "
        "    LIMIT 1"
        "), leased_queue AS ("
        "    UPDATE submission_queue queue_table "
        "    SET "
        "        leased_until = NOW() + ($1 * INTERVAL '1 second'), "
        "        attempt_no = attempt_no + 1, "
        "        lease_token = md5(random()::text || clock_timestamp()::text || queue_table.submission_id::text || (queue_table.attempt_no + 1)::text) "
        "    FROM lease_candidate "
        "    WHERE queue_table.submission_id = lease_candidate.submission_id "
        "    RETURNING "
        "        queue_table.submission_id, "
        "        queue_table.attempt_no, "
        "        queue_table.lease_token, "
        "        queue_table.leased_until::text "
        ") "
        "SELECT "
        "    leased_queue.submission_id, "
        "    lease_candidate.problem_id, "
        "    lease_candidate.problem_version, "
        "    lease_candidate.queue_wait_ms, "
        "    leased_queue.attempt_no, "
        "    leased_queue.lease_token, "
        "    leased_queue.leased_until, "
        "    lease_candidate.language, "
        "    lease_candidate.source_code "
        "FROM leased_queue "
        "JOIN lease_candidate "
        "ON lease_candidate.submission_id = leased_queue.submission_id",
        pqxx::params{lease_duration.count()}
    );

    if(leased_submission_result.empty()){
        return std::optional<submission_domain_dto::leased_submission>{std::nullopt};
    }

    return std::optional<submission_domain_dto::leased_submission>{
        submission_row_mapper::map_leased_submission_row(
            leased_submission_result[0]
        )
    };
}

std::expected<void, repository_error> submission_queue_repository::release_submission_lease(
    pqxx::transaction_base& transaction,
    const submission_domain_dto::leased_submission& leased_submission_value
){
    if(
        leased_submission_value.submission_id <= 0 ||
        leased_submission_value.attempt_no <= 0 ||
        leased_submission_value.lease_token.empty()
    ){
        return std::unexpected(repository_error::invalid_reference);
    }

    const auto update_result = transaction.exec(
        "UPDATE submission_queue "
        "SET "
        "available_at = NOW(), "
        "leased_until = NULL, "
        "lease_token = NULL "
        "WHERE "
        "submission_id = $1 AND "
        "attempt_no = $2 AND "
        "lease_token = $3 AND "
        "leased_until IS NOT NULL AND "
        "leased_until > NOW()",
        pqxx::params{
            leased_submission_value.submission_id,
            leased_submission_value.attempt_no,
            leased_submission_value.lease_token
        }
    );
    if(update_result.affected_rows() == 0){
        return std::unexpected(
            repository_error{
                repository_error_code::conflict,
                "submission lease is stale"
            }
        );
    }

    return {};
}
