#include "error/problem_content_error.hpp"

namespace problem_content_error{
    http_error missing_sample_to_delete(){
        return http_error{
            http_error_code::validation_error,
            "missing sample to delete"
        };
    }
}
