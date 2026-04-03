#include "error/db_error.hpp"

#include <pqxx/pqxx>

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct db_error_spec{
        std::string_view default_message;
        bool is_retryable = false;
        bool should_reconnect = false;
        bool is_constraint_violation = false;
    };

    constexpr db_error_spec unknown_db_error_spec{
        "unknown database error"
    };

    constexpr std::array<db_error_spec, 10> db_error_specs{{
        {"invalid database argument"},
        {"invalid database connection", true, true},
        {"database operation interrupted", true},
        {"database unique violation", false, false, true},
        {"database constraint violation", false, false, true},
        {"database connection broken", true, true},
        {"database serialization failure", true},
        {"database deadlock detected", true},
        {"database unavailable"},
        {"database internal error"},
    }};

    const db_error_spec& describe_db_error(db_error_code ec){
        const auto index = static_cast<std::size_t>(ec);
        if(index >= db_error_specs.size()){
            return unknown_db_error_spec;
        }

        return db_error_specs[index];
    }

    db_error map_psql_exception(const std::exception& exception){
        if(dynamic_cast<const pqxx::serialization_failure*>(&exception) != nullptr){
            return db_error::serialization_failure;
        }
        if(dynamic_cast<const pqxx::deadlock_detected*>(&exception) != nullptr){
            return db_error::deadlock_detected;
        }
        if(dynamic_cast<const pqxx::unique_violation*>(&exception) != nullptr){
            return db_error::unique_violation;
        }
        if(
            dynamic_cast<const pqxx::foreign_key_violation*>(&exception) != nullptr ||
            dynamic_cast<const pqxx::not_null_violation*>(&exception) != nullptr ||
            dynamic_cast<const pqxx::check_violation*>(&exception) != nullptr
        ){
            return db_error::constraint_violation;
        }
        if(dynamic_cast<const pqxx::broken_connection*>(&exception) != nullptr){
            return db_error::broken_connection;
        }

        return db_error::internal;
    }
}

db_error::db_error(
    db_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_db_error(code_value).default_message}
            : std::move(message_value)
    ){}

bool db_error::operator==(const db_error& other) const{
    return code == other.code;
}

bool db_error::is_retryable() const{
    return describe_db_error(code).is_retryable;
}

bool db_error::should_reconnect() const{
    return describe_db_error(code).should_reconnect;
}

bool db_error::is_constraint_violation() const{
    return describe_db_error(code).is_constraint_violation;
}

const db_error db_error::invalid_argument{
    db_error_code::invalid_argument
};
const db_error db_error::invalid_connection{
    db_error_code::invalid_connection
};
const db_error db_error::interrupted{
    db_error_code::interrupted
};
const db_error db_error::unique_violation{
    db_error_code::unique_violation
};
const db_error db_error::constraint_violation{
    db_error_code::constraint_violation
};
const db_error db_error::broken_connection{
    db_error_code::broken_connection
};
const db_error db_error::serialization_failure{
    db_error_code::serialization_failure
};
const db_error db_error::deadlock_detected{
    db_error_code::deadlock_detected
};
const db_error db_error::unavailable{
    db_error_code::unavailable
};
const db_error db_error::internal{
    db_error_code::internal
};

std::string to_string(db_error_code ec){
    return std::string{describe_db_error(ec).default_message};
}

std::string to_string(const db_error& ec){
    return ec.message;
}

db_error db_error::from_psql_exception(const std::exception& exception){
    return map_psql_exception(exception);
}
