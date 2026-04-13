#include "http_handler/problem_content_query_handler.hpp"

#include "application/problem_content_query.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/problem_guard.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    using response_type = problem_content_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_problem_reference_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&)
                -> command_expected<problem_dto::reference> {
                return problem_reference_value;
            },
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_get_limits_spec(std::int64_t problem_id){
        return http_endpoint::make_json_spec(
            make_problem_reference_guard(problem_id),
            http_endpoint::make_db_execute(get_problem_limits_query::execute),
            problem_json_serializer::make_limits_object
        );
    }

    auto make_get_samples_spec(std::int64_t problem_id){
        return http_endpoint::make_json_spec(
            make_problem_reference_guard(problem_id),
            http_endpoint::make_db_execute(list_problem_samples_query::execute),
            problem_json_serializer::make_sample_list_object
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
