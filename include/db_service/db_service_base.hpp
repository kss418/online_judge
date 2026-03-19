#pragma once
#include "common/error_code.hpp"
#include "common/db_connection.hpp"

#include <expected>
#include <utility>

template <typename derived_type>
class db_service_base{
public:
    static std::expected<derived_type, error_code> create(db_connection db_connection){
        if(!db_connection.is_connected()){
            return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
        }

        return derived_type(std::move(db_connection));
    }

    pqxx::connection& connection(){
        return db_connection_.connection();
    }

    const pqxx::connection& connection() const{
        return db_connection_.connection();
    }

protected:
    explicit db_service_base(db_connection db_connection) :
        db_connection_(std::move(db_connection)){}

    bool is_connected() const{
        return db_connection_.is_connected();
    }

private:
    db_connection db_connection_;
};
