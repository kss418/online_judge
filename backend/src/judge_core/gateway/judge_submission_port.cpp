#include "judge_core/gateway/judge_submission_port.hpp"

#include "db_service/submission_service.hpp"

#include <utility>

std::expected<judge_submission_port, judge_error> judge_submission_port::create(
    const db_connection_config& db_config
){
    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return judge_submission_port(std::move(*db_connection_exp));
}

judge_submission_port::judge_submission_port(
    db_connection db_connection_value
) :
    db_connection_(std::move(db_connection_value)){}

judge_submission_port::judge_submission_port(
    judge_submission_port&& other
) noexcept = default;

judge_submission_port& judge_submission_port::operator=(
    judge_submission_port&& other
) noexcept = default;

judge_submission_port::~judge_submission_port() = default;

std::expected<void, judge_error> judge_submission_port::mark_judging(
    std::int64_t submission_id
){
    const auto mark_judging_exp = submission_service::mark_judging(
        db_connection_,
        submission_id
    );
    if(!mark_judging_exp){
        return std::unexpected(judge_error{mark_judging_exp.error()});
    }

    return {};
}

std::expected<void, judge_error> judge_submission_port::finalize_submission(
    const submission_dto::finalize_request& finalize_request_value
){
    const auto finalize_submission_exp = submission_service::finalize_submission(
        db_connection_,
        finalize_request_value
    );
    if(!finalize_submission_exp){
        return std::unexpected(judge_error{finalize_submission_exp.error()});
    }

    return {};
}

std::expected<void, judge_error> judge_submission_port::requeue_submission_immediately(
    std::int64_t submission_id,
    std::optional<std::string> reason_opt
){
    const auto requeue_submission_exp =
        submission_service::requeue_submission_immediately(
            db_connection_,
            submission_id,
            std::move(reason_opt)
        );
    if(!requeue_submission_exp){
        return std::unexpected(judge_error{requeue_submission_exp.error()});
    }

    return {};
}
