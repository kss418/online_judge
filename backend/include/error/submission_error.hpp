#pragma once

#include "error/http_error.hpp"

namespace submission_error{
    http_error submission_banned();
    http_error source_access_denied();
}
