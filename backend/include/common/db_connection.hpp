#pragma once
#include "common/error_code.hpp"

#include <pqxx/pqxx>
#include <expected>
#include <memory>
#include <string>

class db_connection{
    static std::expected<std::string, error_code> initialize();
    std::expected<void, error_code> connect(const std::string& connection_string_value);
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> connection_;
public:
    db_connection() = default;
    db_connection(const db_connection&) = delete;
    db_connection& operator=(const db_connection&) = delete;
    db_connection(db_connection&&) noexcept = default;
    db_connection& operator=(db_connection&&) noexcept = default;

    static std::expected<db_connection, error_code> create();
    std::expected<void, error_code> reconnect();
    void disconnect();
    bool is_connected() const;
    const std::string& connection_string() const;
    pqxx::connection& connection();
    const pqxx::connection& connection() const;
};
