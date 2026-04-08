#include "judge_core/gateway/judge_submission_facade.hpp"

#include "db_service/submission_service.hpp"

#include <utility>

std::expected<judge_submission_facade, judge_error> judge_submission_facade::create(
    const db_connection_config& db_config
){
    auto db_connection_exp = db_connection::create(db_config);
    if(!db_connection_exp){
        return std::unexpected(judge_error{db_connection_exp.error()});
    }

    return judge_submission_facade(std::move(*db_connection_exp));
}

judge_submission_facade::judge_submission_facade(
    db_connection db_connection_value
) :
    db_connection_(std::move(db_connection_value)){}

judge_submission_facade::judge_submission_facade(
    judge_submission_facade&& other
) noexcept = default;

judge_submission_facade& judge_submission_facade::operator=(
    judge_submission_facade&& other
) noexcept = default;

judge_submission_facade::~judge_submission_facade() = default;

std::expected<void, judge_error> judge_submission_facade::mark_judging(
    const submission_dto::leased_submission& leased_submission_value
){
    const auto mark_judging_exp = submission_service::mark_judging(
        db_connection_,
        leased_submission_value
    );
    if(!mark_judging_exp){
        return std::unexpected(judge_error{mark_judging_exp.error()});
    }

    return {};
}

std::expected<void, judge_error> judge_submission_facade::finalize_submission(
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

std::expected<void, judge_error> judge_submission_facade::requeue_submission_immediately(
    const submission_dto::leased_submission& leased_submission_value,
    std::optional<std::string> reason_opt
){
    const auto requeue_submission_exp =
        submission_service::requeue_submission_immediately(
            db_connection_,
            leased_submission_value,
            std::move(reason_opt)
        );
    if(!requeue_submission_exp){
        return std::unexpected(judge_error{requeue_submission_exp.error()});
    }

    return {};
}
