#include "common/env_utility.hpp"

#include <cstdlib>

std::expected<std::string, error_code> env_utility::require_env(const char* key){
    const char* value = std::getenv(key);
    if(value == nullptr || *value == '\0'){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::string(value);
}

std::expected<void, error_code> env_utility::require_all_envs(){
    return require_envs(
        {
            "HTTP_PORT",
            "DB_USER",
            "DB_PASSWORD",
            "DB_HOST",
            "DB_PORT",
            "DB_NAME",
            "DB_ADMIN_USER",
            "DB_ADMIN_PASSWORD",
            "WORKER_ID",
            "MAX_CONCURRENT_JOBS",
            "JUDGE_SOURCE_ROOT",
            "TESTCASE_PATH",
            "JUDGE_CPP_COMPILER_PATH",
            "JUDGE_PYTHON_PATH",
            "JUDGE_JAVA_RUNTIME_PATH"
        }
    );
}

std::expected<void, error_code> env_utility::require_envs(std::initializer_list<const char*> keys){
    for(const char* key : keys){
        const auto env_value_exp = require_env(key);
        if(!env_value_exp){
            return std::unexpected(env_value_exp.error());
        }
    }

    return {};
}
