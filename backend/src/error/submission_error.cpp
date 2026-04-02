#include "error/submission_error.hpp"

namespace submission_error{
    http_error submission_banned(){
        return http_error{http_error_code::submission_banned};
    }
}
