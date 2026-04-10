#include "http_core/http_runtime_config.hpp"

#include "common/string_util.hpp"

#include <cstdlib>
#include <limits>

namespace{
    infra_error make_invalid_http_runtime_config_error(const char* key){
        return infra_error{
            infra_error_code::invalid_argument,
            std::string{"invalid "} + key
        };
    }

    std::expected<std::int64_t, infra_error> parse_nonnegative_int64_env(const char* key){
        const char* value = std::getenv(key);
        if(value == nullptr || *value == '\0'){
            return std::unexpected(make_invalid_http_runtime_config_error(key));
        }

        if(value[0] == '0' && value[1] == '\0'){
            return std::int64_t{0};
        }

        const auto parsed_value_opt = string_util::parse_positive_int64(value);
        if(!parsed_value_opt){
            return std::unexpected(make_invalid_http_runtime_config_error(key));
        }

        return *parsed_value_opt;
    }

    std::expected<std::size_t, infra_error> parse_optional_positive_size_t_env(
        const char* key,
        std::size_t default_value
    ){
        const char* value = std::getenv(key);
        if(value == nullptr || *value == '\0'){
            return default_value;
        }

        const auto parsed_value_opt = string_util::parse_positive_int64(value);
        if(
            !parsed_value_opt ||
            static_cast<std::uint64_t>(*parsed_value_opt) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
        ){
            return std::unexpected(make_invalid_http_runtime_config_error(key));
        }

        return static_cast<std::size_t>(*parsed_value_opt);
    }

    std::expected<std::optional<std::size_t>, infra_error> parse_optional_queue_limit_env(
        const char* key,
        std::optional<std::size_t> default_value_opt
    ){
        const char* value = std::getenv(key);
        if(value == nullptr || *value == '\0'){
            return default_value_opt;
        }

        const auto parsed_value_exp = parse_nonnegative_int64_env(key);
        if(
            !parsed_value_exp ||
            static_cast<std::uint64_t>(*parsed_value_exp) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
        ){
            return std::unexpected(make_invalid_http_runtime_config_error(key));
        }

        if(*parsed_value_exp == 0){
            return std::optional<std::size_t>{};
        }

        return std::optional<std::size_t>{static_cast<std::size_t>(*parsed_value_exp)};
    }

    std::expected<std::optional<std::chrono::milliseconds>, infra_error>
    parse_optional_timeout_ms_env(
        const char* key,
        std::chrono::milliseconds default_value
    ){
        const char* value = std::getenv(key);
        if(value == nullptr || *value == '\0'){
            return std::optional<std::chrono::milliseconds>{default_value};
        }

        const auto parsed_value_exp = parse_nonnegative_int64_env(key);
        if(!parsed_value_exp){
            return std::unexpected(parsed_value_exp.error());
        }

        if(*parsed_value_exp == 0){
            return std::optional<std::chrono::milliseconds>{};
        }

        return std::optional<std::chrono::milliseconds>{
            std::chrono::milliseconds{*parsed_value_exp}
        };
    }

    std::expected<std::chrono::milliseconds, infra_error>
    parse_nonnegative_timeout_ms_env(
        const char* key,
        std::chrono::milliseconds default_value
    ){
        const char* value = std::getenv(key);
        if(value == nullptr || *value == '\0'){
            return default_value;
        }

        const auto parsed_value_exp = parse_nonnegative_int64_env(key);
        if(!parsed_value_exp){
            return std::unexpected(parsed_value_exp.error());
        }

        return std::chrono::milliseconds{*parsed_value_exp};
    }
}

std::expected<http_runtime_config, infra_error> http_runtime_config::load(
    std::size_t default_http_worker_count
){
    http_runtime_config config;

    const char* http_port_text = std::getenv("HTTP_PORT");
    if(http_port_text == nullptr || *http_port_text == '\0'){
        return std::unexpected(make_invalid_http_runtime_config_error("HTTP_PORT"));
    }

    const auto port_opt = string_util::parse_positive_int16(http_port_text);
    if(!port_opt){
        return std::unexpected(make_invalid_http_runtime_config_error("HTTP_PORT"));
    }
    config.port = *port_opt;

    const auto io_worker_count_exp = parse_optional_positive_size_t_env(
        "HTTP_WORKER_COUNT",
        default_http_worker_count
    );
    if(!io_worker_count_exp){
        return std::unexpected(io_worker_count_exp.error());
    }
    config.io_worker_count = *io_worker_count_exp;

    const auto handler_worker_count_exp = parse_optional_positive_size_t_env(
        "HTTP_HANDLER_WORKER_COUNT",
        config.io_worker_count
    );
    if(!handler_worker_count_exp){
        return std::unexpected(handler_worker_count_exp.error());
    }
    config.handler_worker_count = *handler_worker_count_exp;

    const auto db_pool_size_exp = parse_optional_positive_size_t_env(
        "HTTP_DB_POOL_SIZE",
        config.io_worker_count
    );
    if(!db_pool_size_exp){
        return std::unexpected(db_pool_size_exp.error());
    }
    config.db_pool_size = *db_pool_size_exp;

    if(
        config.handler_worker_count >
        std::numeric_limits<std::size_t>::max() / std::size_t{4}
    ){
        return std::unexpected(
            make_invalid_http_runtime_config_error("HTTP_HANDLER_QUEUE_LIMIT")
        );
    }
    const std::size_t default_handler_queue_limit = config.handler_worker_count * std::size_t{4};

    const auto handler_queue_limit_exp = parse_optional_queue_limit_env(
        "HTTP_HANDLER_QUEUE_LIMIT",
        std::optional<std::size_t>{default_handler_queue_limit}
    );
    if(!handler_queue_limit_exp){
        return std::unexpected(handler_queue_limit_exp.error());
    }
    config.handler_queue_limit_opt = *handler_queue_limit_exp;

    const auto db_acquire_timeout_exp = parse_optional_timeout_ms_env(
        "HTTP_DB_ACQUIRE_TIMEOUT_MS",
        std::chrono::milliseconds{100}
    );
    if(!db_acquire_timeout_exp){
        return std::unexpected(db_acquire_timeout_exp.error());
    }
    config.db_acquire_timeout_opt = *db_acquire_timeout_exp;

    const auto request_deadline_exp = parse_optional_timeout_ms_env(
        "HTTP_REQUEST_DEADLINE_MS",
        std::chrono::milliseconds{30000}
    );
    if(!request_deadline_exp){
        return std::unexpected(request_deadline_exp.error());
    }
    config.request_deadline_opt = *request_deadline_exp;

    const auto keep_alive_idle_timeout_exp = parse_optional_timeout_ms_env(
        "HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS",
        std::chrono::milliseconds{15000}
    );
    if(!keep_alive_idle_timeout_exp){
        return std::unexpected(keep_alive_idle_timeout_exp.error());
    }
    config.keep_alive_idle_timeout_opt = *keep_alive_idle_timeout_exp;

    const auto judge_heartbeat_stale_after_exp = parse_nonnegative_timeout_ms_env(
        "JUDGE_HEARTBEAT_STALE_AFTER_MS",
        std::chrono::milliseconds{15000}
    );
    if(!judge_heartbeat_stale_after_exp){
        return std::unexpected(judge_heartbeat_stale_after_exp.error());
    }
    config.judge_heartbeat_stale_after = *judge_heartbeat_stale_after_exp;

    return config;
}
