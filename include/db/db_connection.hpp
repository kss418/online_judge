#pragma once
#include "common/error_code.hpp"

#include <pqxx/pqxx>
#include <expected>
#include <memory>
#include <string>

class db_connection{
    static std::expected<std::string, error_code> initialize();
    std::string connection_string_;
    std::unique_ptr <pqxx::connection> connection_;
public:
    static std::expected<db_connection, error_code> create();
    void disconnect();
    bool is_connected() const;
    const std::string& connection_string() const;
    pqxx::connection& connection();
    const pqxx::connection& connection() const;
};
