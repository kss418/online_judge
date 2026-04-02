#include "error/submission_error.hpp"

namespace submission_error{
    http_error submission_banned(){
        return http_error{http_error_code::submission_banned};
    }

    http_error source_access_denied(){
        return http_error{
            http_error_code::forbidden,
            "submission source access denied"
        };
    }
}
