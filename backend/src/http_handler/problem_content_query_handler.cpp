#include "http_handler/problem_content_query_handler.hpp"

#include "db_service/problem_content_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

problem_content_query_handler::response_type problem_content_query_handler::get_limits(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [&context, &problem_reference_value]() -> response_type {
            const auto limits_exp = problem_content_service::get_limits(
                context.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::json(
                context.request,
                std::move(limits_exp),
                [](const auto& limits_value){
                    boost::json::object response_object;
                    response_object["memory_limit_mb"] = limits_value.memory_mb;
                    response_object["time_limit_ms"] = limits_value.time_ms;
                    return response_object;
                }
            );
        },
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_query_handler::response_type problem_content_query_handler::get_samples(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [&context, &problem_reference_value]() -> response_type {
            const auto sample_values_exp = problem_content_service::list_samples(
                context.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::json(
                context.request,
                std::move(sample_values_exp),
                problem_json_serializer::make_sample_list_object
            );
        },
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
