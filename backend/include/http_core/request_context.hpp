#pragma once

#include "common/db_connection_pool.hpp"
#include "dto/auth_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_router/operation_kind.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

class request_observer;

struct request_context{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    explicit request_context(
        const request_type& request_value,
        std::string request_id_value = {},
        request_observer* observer_value = nullptr
    ) :
        request(request_value),
        request_id(std::move(request_id_value)),
        observer(observer_value){}

    request_context(
        const request_type& request_value,
        db_connection_pool::lease db_connection_lease_value,
        std::string request_id_value = {},
        request_observer* observer_value = nullptr
    ) :
        request(request_value),
        request_id(std::move(request_id_value)),
        observer(observer_value),
        db_connection_lease_opt(std::move(db_connection_lease_value)){
        if(db_connection_lease_opt.has_value()){
            db_connection_ptr_ = &db_connection_lease_opt->connection();
        }
    }

    bool has_db_connection() const{
        return db_connection_ptr_ != nullptr;
    }

    void clear_endpoint_metadata(){
        endpoint_name_opt.reset();
        operation_kind_opt.reset();
    }

    void set_endpoint_metadata(
        std::string_view endpoint_name_value,
        http_route::operation_kind operation_kind_value
    ){
        endpoint_name_opt = endpoint_name_value;
        operation_kind_opt = operation_kind_value;
    }

    db_connection& db_connection_ref(){
        return *db_connection_ptr_;
    }

    const db_connection& db_connection_ref() const{
        return *db_connection_ptr_;
    }

    const request_type& request;
    std::string request_id;
    std::optional<db_connection_pool::lease> db_connection_lease_opt;
    std::optional<auth_dto::identity> auth_identity_opt;
    std::optional<std::string_view> endpoint_name_opt;
    std::optional<http_route::operation_kind> operation_kind_opt;
    request_observer* observer = nullptr;

private:
    db_connection* db_connection_ptr_ = nullptr;
};
