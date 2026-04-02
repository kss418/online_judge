#pragma once

#include <string>

struct error_code;
struct infra_error;
struct pool_error;

enum class http_server_error_code{
    invalid_configuration,
    unavailable,
    internal
};

struct http_server_error{
    http_server_error_code code;
    std::string message;

    http_server_error(http_server_error_code code_value, std::string message_value = {});
    http_server_error(const infra_error& error);
    http_server_error(const pool_error& error);
    http_server_error(const error_code& error);

    bool operator==(const http_server_error& other) const;

    static const http_server_error invalid_configuration;
    static const http_server_error unavailable;
    static const http_server_error internal;

    static http_server_error from_infra_error(const infra_error& error);
    static http_server_error from_pool_error(const pool_error& error);
    static http_server_error from_error_code(const error_code& error);
};

std::string to_string(http_server_error_code error);
std::string to_string(const http_server_error& error);
