#include "http_handler/problem_content_query_handler.hpp"

#include "db_service/problem_content_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_get_limits_spec(std::int64_t problem_id){
        const problem_dto::reference problem_reference_value{problem_id};

        return http_handler_spec::make_path_value_json_spec(
            problem_reference_value,
            problem_content_service::get_limits,
            problem_json_serializer::make_limits_object,
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_get_samples_spec(std::int64_t problem_id){
        const problem_dto::reference problem_reference_value{problem_id};

        return http_handler_spec::make_path_value_json_spec(
            problem_reference_value,
            problem_content_service::list_samples,
            problem_json_serializer::make_sample_list_object,
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }
}

problem_content_query_handler::response_type problem_content_query_handler::get_limits(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_get_limits_spec(problem_id));
}

problem_content_query_handler::response_type problem_content_query_handler::get_samples(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_get_samples_spec(problem_id));
}
