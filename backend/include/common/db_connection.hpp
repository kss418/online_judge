#pragma once
#include "error/db_error.hpp"
#include "error/infra_error.hpp"

#include <pqxx/pqxx>
#include <expected>
#include <memory>
#include <string>

struct db_connection_config{
    std::string connection_string;
};

class db_connection{
    std::expected<void, db_error> connect(const std::string& connection_string_value);
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> connection_;
public:
    db_connection() = default;
    db_connection(const db_connection&) = delete;
    db_connection& operator=(const db_connection&) = delete;
    db_connection(db_connection&&) noexcept = default;
    db_connection& operator=(db_connection&&) noexcept = default;

    static std::expected<db_connection_config, infra_error> load_db_connection_config();
    static std::expected<db_connection, db_error> create(
        const db_connection_config& config
    );
    std::expected<void, db_error> reconnect();
    void disconnect();
    bool is_connected() const;
    const std::string& connection_string() const;
    pqxx::connection& connection();
    const pqxx::connection& connection() const;
};
