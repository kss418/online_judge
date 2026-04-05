#pragma once

#include <string>

struct compile_failure{
    int exit_code = 0;
    std::string stderr_text;
};
