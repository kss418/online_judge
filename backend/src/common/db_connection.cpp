#include "common/db_connection.hpp"
#include "common/env_util.hpp"

#include "error/error_code.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

std::expected<db_connection_config, infra_error> db_connection::load_db_connection_config(){
    const auto env_values_exp = env_util::require_envs(
        {"DB_HOST", "DB_NAME", "DB_USER", "DB_PASSWORD", "DB_PORT"}
    );
    if(!env_values_exp){
        return std::unexpected(env_values_exp.error());
    }

    const auto& env_values = *env_values_exp;
    const std::string& host = env_values[0];
    const std::string& database = env_values[1];
    const std::string& user = env_values[2];
    const std::string& password = env_values[3];
    const std::string& port_string = env_values[4];

    unsigned int port = 0;
    const auto [parse_end, parse_error] = std::from_chars(
        port_string.data(), port_string.data() + port_string.size(), port
    );

    if(
        parse_error != std::errc{} ||
        parse_end != port_string.data() + port_string.size() ||
        port == 0 ||
        port > std::numeric_limits<std::uint16_t>::max()
    ){
        return std::unexpected(infra_error::invalid_argument);
    }

    db_connection_config config;
    config.connection_string = "postgresql://";
    config.connection_string += user;
    config.connection_string += ":";
    config.connection_string += password;
    config.connection_string += "@";
    config.connection_string += host;
    config.connection_string += ":";
    config.connection_string += std::to_string(port);
    config.connection_string += "/";
    config.connection_string += database;
    return config;
}

std::expected<db_connection, db_error> db_connection::create(
    const db_connection_config& config
){
    db_connection connection;
    auto connect_exp = connection.connect(config.connection_string);
    if(!connect_exp){
        return std::unexpected(connect_exp.error());
    }

    return connection;
}

std::expected<void, db_error> db_connection::connect(
    const std::string& connection_string_value
){
    connection_.reset();
    connection_string_ = connection_string_value;

    try{
        auto created_connection = std::make_unique<pqxx::connection>(
            connection_string_
        );
        if(!created_connection->is_open()){
            connection_.reset();
            return std::unexpected(db_error::broken_connection);
        }

        connection_ = std::move(created_connection);
        return {};
    }
    catch(const std::exception& exception){
        connection_.reset();
        return std::unexpected(
            db_error::from_error_code(error_code::map_psql_error_code(exception))
        );
    }
}

std::expected<void, db_error> db_connection::reconnect(){
    if(connection_string_.empty()){
        return std::unexpected(db_error::invalid_connection);
    }

    return connect(connection_string_);
}

void db_connection::disconnect(){
    connection_.reset();
    connection_string_.clear();
}

bool db_connection::is_connected() const{
    return connection_ && connection_->is_open();
}

const std::string& db_connection::connection_string() const{
    return connection_string_;
}

pqxx::connection& db_connection::connection(){
    return *connection_;
}

const pqxx::connection& db_connection::connection() const{
    return *connection_;
}
