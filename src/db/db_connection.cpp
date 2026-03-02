#include "db/db_connection.hpp"
#include "common/env_utility.hpp"

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
    connection.connection_string_ = std::move(*connection_string_exp);
    try{
        auto created_connection = std::make_unique<pqxx::connection>(connection.connection_string_);
        if(!created_connection->is_open()){
            return std::unexpected(error_code::create(errno_error::io_error));
        }

        connection.connection_ = std::move(created_connection);
    }
    catch(const pqxx::broken_connection&){
        return std::unexpected(error_code::create(errno_error::io_error));
    }
    catch(const pqxx::sql_error&){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }
    catch(const std::exception&){
        return std::unexpected(error_code::create(errno_error::unknown_error));
    }

    return connection;
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

std::expected<std::string, error_code> db_connection::initialize(){
    auto host_exp = env_utility::require_env("DB_HOST");
    auto database_exp = env_utility::require_env("DB_NAME");
    auto user_exp = env_utility::require_env("DB_USER");
    auto password_exp = env_utility::require_env("DB_PASSWORD");
    auto port_exp = env_utility::require_env("DB_PORT");

    if(!host_exp || !database_exp || !user_exp || !password_exp || !port_exp){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

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
