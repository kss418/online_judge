#include "common/db_connection.hpp"
#include "common/env_util.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

std::expected<db_connection, error_code> db_connection::create(){
    auto connection_string_exp = initialize();
    if(!connection_string_exp){
        return std::unexpected(connection_string_exp.error());
    }

    db_connection connection;
    auto connect_exp = connection.connect(*connection_string_exp);
    if(!connect_exp){
        return std::unexpected(connect_exp.error());
    }

    return connection;
}

std::expected<void, error_code> db_connection::connect(
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
            return std::unexpected(error_code::create(psql_error::broken_connection));
        }

        connection_ = std::move(created_connection);
        return {};
    }
    catch(const std::exception& exception){
        connection_.reset();
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> db_connection::reconnect(){
    if(connection_string_.empty()){
        auto connection_string_exp = initialize();
        if(!connection_string_exp){
            return std::unexpected(connection_string_exp.error());
        }

        return connect(*connection_string_exp);
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

std::expected<std::string, error_code> db_connection::initialize(){
    const auto require_envs_exp = env_util::require_envs(
        {"DB_HOST", "DB_NAME", "DB_USER", "DB_PASSWORD", "DB_PORT"}
    );
    if(!require_envs_exp){
        return std::unexpected(require_envs_exp.error());
    }

    auto host_exp = env_util::require_env("DB_HOST");
    auto database_exp = env_util::require_env("DB_NAME");
    auto user_exp = env_util::require_env("DB_USER");
    auto password_exp = env_util::require_env("DB_PASSWORD");
    auto port_exp = env_util::require_env("DB_PORT");

    const std::string port_string(*port_exp);

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
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::string connection_string = "postgresql://";
    connection_string += *user_exp;
    connection_string += ":";
    connection_string += *password_exp;
    connection_string += "@";
    connection_string += *host_exp;
    connection_string += ":";
    connection_string += std::to_string(port);
    connection_string += "/";
    connection_string += *database_exp;
    return connection_string;
}
