#include "db_service/submission_core_service.hpp"
#include "db_util/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

std::expected<std::int64_t, error_code> submission_core_service::create_submission(
    db_connection& connection,
    std::int64_t user_id,
    std::int64_t problem_id,
    const submission_dto::source& source_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        user_id <= 0 ||
        problem_id <= 0 ||
        source_value.language.empty() ||
        source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_submission_exp = submission_util::create_submission(
            transaction,
            user_id,
            problem_id,
            source_value
        );
        if(!create_submission_exp){
            return std::unexpected(create_submission_exp.error());
        }

        transaction.commit();
        return create_submission_exp.value();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<submission_dto::summary>, error_code>
submission_core_service::list_submissions(
    db_connection& connection,
    const submission_dto::list_filter& filter_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        (filter_value.top_submission_id_opt && *filter_value.top_submission_id_opt <= 0) ||
        (filter_value.user_id_opt && *filter_value.user_id_opt <= 0) ||
        (filter_value.problem_id_opt && *filter_value.problem_id_opt <= 0) ||
        (filter_value.status_opt && !parse_submission_status(*filter_value.status_opt))
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());

        std::string submission_list_query =
            "SELECT "
            "submission_id, "
            "user_id, "
            "problem_id, "
            "language, "
            "status::text, "
            "score, "
            "created_at::text, "
            "updated_at::text "
            "FROM submissions "
            "WHERE 1 = 1";
        pqxx::params query_params;
        int query_param_index = 1;

        if(filter_value.top_submission_id_opt){
            submission_list_query +=
                " AND submission_id <= $" + std::to_string(query_param_index++);
            query_params.append(*filter_value.top_submission_id_opt);
        }
        if(filter_value.user_id_opt){
            submission_list_query +=
                " AND user_id = $" + std::to_string(query_param_index++);
            query_params.append(*filter_value.user_id_opt);
        }
        if(filter_value.problem_id_opt){
            submission_list_query +=
                " AND problem_id = $" + std::to_string(query_param_index++);
            query_params.append(*filter_value.problem_id_opt);
        }
        if(filter_value.status_opt){
            submission_list_query +=
                " AND status = $" + std::to_string(query_param_index++) + "::submission_status";
            query_params.append(*filter_value.status_opt);
        }

        submission_list_query += " ORDER BY submission_id DESC LIMIT 20";

        const auto submission_summary_query = transaction.exec(
            submission_list_query,
            query_params
        );

        std::vector<submission_dto::summary> summary_values;
        summary_values.reserve(submission_summary_query.size());
        for(const auto& submission_summary_row : submission_summary_query){
            submission_dto::summary summary_value;
            summary_value.submission_id = submission_summary_row[0].as<std::int64_t>();
            summary_value.user_id = submission_summary_row[1].as<std::int64_t>();
            summary_value.problem_id = submission_summary_row[2].as<std::int64_t>();
            summary_value.language = submission_summary_row[3].as<std::string>();
            summary_value.status = submission_summary_row[4].as<std::string>();
            if(!submission_summary_row[5].is_null()){
                summary_value.score = submission_summary_row[5].as<std::int16_t>();
            }
            summary_value.created_at = submission_summary_row[6].as<std::string>();
            summary_value.updated_at = submission_summary_row[7].as<std::string>();
            summary_values.push_back(std::move(summary_value));
        }

        return summary_values;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
