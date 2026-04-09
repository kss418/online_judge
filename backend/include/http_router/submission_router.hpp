#pragma once

#include "http_handler/submission_handler.hpp"

#include <cstdint>
#include <string_view>

class submission_router{
public:
    using context_type = submission_handler::context_type;
    using request_type = submission_handler::request_type;
    using response_type = submission_handler::response_type;

    response_type route(context_type& context, std::string_view path);

private:
    response_type handle_submissions(context_type& context);
    response_type handle_submission_status_batch(context_type& context);
    response_type handle_submission(
        context_type& context,
        std::int64_t resource_id
    );
    response_type handle_submission_history(
        context_type& context,
        std::int64_t resource_id
    );
    response_type handle_submission_source(
        context_type& context,
        std::int64_t resource_id
    );
    response_type handle_submission_rejudge(
        context_type& context,
        std::int64_t resource_id
    );
};
