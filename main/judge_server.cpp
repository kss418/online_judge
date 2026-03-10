#include "common/env_util.hpp"
#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "db/submission_service.hpp"
#include "judge_server/judge_worker.hpp"

#include <expected>
#include <iostream>
#include <utility>

std::expected<submission_service, error_code> create_submission_service(){
    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    auto submission_service_exp = submission_service::create(std::move(*db_connection_exp));
    if(!submission_service_exp){
        return std::unexpected(submission_service_exp.error());
    }

    return std::move(*submission_service_exp);
}

int main(){
    const auto require_all_envs_exp = env_utility::require_all_envs();
    if(!require_all_envs_exp){
        std::cerr << "required environment variables are missing\n";
        return 1;
    }

    auto submission_service_exp = create_submission_service();
    if(!submission_service_exp){
        std::cerr << "failed to initialize submission_service: "
                  << to_string(submission_service_exp.error()) << '\n';
        return 1;
    }

    auto judge_worker_exp = judge_worker::create(std::move(*submission_service_exp));
    if(!judge_worker_exp){
        std::cerr << "failed to initialize judge_worker: " << to_string(judge_worker_exp.error()) << '\n';
        return 1;
    }

    auto run_exp = judge_worker_exp->run();
    if(!run_exp){
        std::cerr << "judge_worker run failed: " << to_string(run_exp.error()) << '\n';
        return 1;
    }

    return 0;
}
