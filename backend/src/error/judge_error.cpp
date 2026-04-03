#include "error/judge_error.hpp"

#include "error/db_error.hpp"
#include "error/error_code.hpp"
#include "error/infra_error.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace{
    struct judge_error_spec{
        std::string_view default_message;
    };

    constexpr judge_error_spec unknown_judge_error_spec{
        "unknown judge error"
    };

    constexpr std::array<judge_error_spec, 5> judge_error_specs{{
        {"validation error"},
        {"not found"},
        {"conflict"},
        {"unavailable"},
        {"internal"},
    }};

    const judge_error_spec& describe_judge_error(judge_error_code code){
        const auto index = static_cast<std::size_t>(code);
        if(index >= judge_error_specs.size()){
            return unknown_judge_error_spec;
        }

        return judge_error_specs[index];
    }

    template<typename... Candidates>
    constexpr bool matches_any(const error_code& error, Candidates... candidates){
        return ((error == candidates) || ...);
    }

    judge_error_code map_db_error_code(const db_error& ec){
        switch(ec.code){
            case db_error_code::invalid_argument:
            case db_error_code::constraint_violation:
                return judge_error_code::validation_error;
            case db_error_code::unique_violation:
                return judge_error_code::conflict;
            case db_error_code::invalid_connection:
            case db_error_code::interrupted:
            case db_error_code::broken_connection:
            case db_error_code::serialization_failure:
            case db_error_code::deadlock_detected:
            case db_error_code::unavailable:
                return judge_error_code::unavailable;
            case db_error_code::internal:
                return judge_error_code::internal;
        }

        return judge_error_code::internal;
    }

    judge_error_code map_infra_error_code(const infra_error& ec){
        switch(ec.code){
            case infra_error_code::invalid_argument:
                return judge_error_code::validation_error;
            case infra_error_code::not_found:
                return judge_error_code::not_found;
            case infra_error_code::conflict:
                return judge_error_code::conflict;
            case infra_error_code::unavailable:
                return judge_error_code::unavailable;
            case infra_error_code::permission_denied:
            case infra_error_code::internal:
                return judge_error_code::internal;
        }

        return judge_error_code::internal;
    }

    judge_error_code map_service_error_code(const service_error& ec){
        switch(ec.code){
            case service_error_code::validation_error:
                return judge_error_code::validation_error;
            case service_error_code::not_found:
                return judge_error_code::not_found;
            case service_error_code::conflict:
                return judge_error_code::conflict;
            case service_error_code::unavailable:
                return judge_error_code::unavailable;
            case service_error_code::unauthorized:
            case service_error_code::forbidden:
            case service_error_code::internal:
                return judge_error_code::internal;
        }

        return judge_error_code::internal;
    }

    judge_error_code map_error_code_kind(const error_code& ec){
        if(matches_any(
            ec,
            errno_error::invalid_argument,
            psql_error::foreign_key_violation,
            psql_error::not_null_violation,
            psql_error::check_violation
        )){
            return judge_error_code::validation_error;
        }

        if(ec == errno_error::file_not_found){
            return judge_error_code::not_found;
        }

        if(matches_any(
            ec,
            psql_error::unique_violation,
            errno_error::file_exists
        )){
            return judge_error_code::conflict;
        }

        if(matches_any(
            ec,
            errno_error::invalid_file_descriptor,
            errno_error::interrupted_system_call,
            errno_error::resource_temporarily_unavailable,
            boost_error::timed_out,
            boost_error::would_block,
            boost_error::try_again,
            psql_error::broken_connection,
            psql_error::serialization_failure,
            psql_error::deadlock_detected
        )){
            return judge_error_code::unavailable;
        }

        return judge_error_code::internal;
    }
}

judge_error::judge_error(
    judge_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_judge_error(code_value).default_message}
            : std::move(message_value)
    ){}

judge_error::judge_error(const db_error& ec) :
    judge_error(map_db_error_code(ec), ec.message){}

judge_error::judge_error(const service_error& ec) :
    judge_error(map_service_error_code(ec), ec.message){}

judge_error::judge_error(const error_code& ec) :
    judge_error(map_error_code_kind(ec), to_string(ec)){}

judge_error::judge_error(const infra_error& ec) :
    judge_error(map_infra_error_code(ec), ec.message){}

bool judge_error::operator==(const judge_error& other) const{
    return code == other.code;
}

const judge_error judge_error::validation_error{
    judge_error_code::validation_error
};
const judge_error judge_error::not_found{
    judge_error_code::not_found
};
const judge_error judge_error::conflict{
    judge_error_code::conflict
};
const judge_error judge_error::unavailable{
    judge_error_code::unavailable
};
const judge_error judge_error::internal{
    judge_error_code::internal
};

std::string to_string(judge_error_code ec){
    return std::string{describe_judge_error(ec).default_message};
}

std::string to_string(const judge_error& ec){
    return ec.message;
}
