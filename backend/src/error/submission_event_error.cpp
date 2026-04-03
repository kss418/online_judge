#include "error/submission_event_error.hpp"

#include "error/db_error.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct submission_event_error_spec{
        std::string_view default_message;
    };

    constexpr submission_event_error_spec unknown_submission_event_error_spec{
        "unknown submission event error"
    };

    constexpr std::array<submission_event_error_spec, 3> submission_event_error_specs{{
        {"invalid submission event argument"},
        {"submission event unavailable"},
        {"submission event internal error"},
    }};

    const submission_event_error_spec& describe_submission_event_error(
        submission_event_error_code error
    ){
        const auto index = static_cast<std::size_t>(error);
        if(index >= submission_event_error_specs.size()){
            return unknown_submission_event_error_spec;
        }

        return submission_event_error_specs[index];
    }
}

submission_event_error::submission_event_error(
    submission_event_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_submission_event_error(code_value).default_message}
            : std::move(message_value)
    ){}

submission_event_error::submission_event_error(const db_error& error)
:
    submission_event_error(from_db_error(error)){}

bool submission_event_error::operator==(const submission_event_error& other) const{
    return code == other.code;
}

submission_event_error submission_event_error::from_db_error(const db_error& error){
    switch(error.code){
        case db_error_code::invalid_argument:
            return submission_event_error{
                submission_event_error_code::invalid_argument,
                error.message
            };
        case db_error_code::invalid_connection:
        case db_error_code::interrupted:
        case db_error_code::broken_connection:
        case db_error_code::serialization_failure:
        case db_error_code::deadlock_detected:
        case db_error_code::unavailable:
            return submission_event_error{
                submission_event_error_code::unavailable,
                error.message
            };
        case db_error_code::unique_violation:
        case db_error_code::constraint_violation:
        case db_error_code::internal:
            return submission_event_error{
                submission_event_error_code::internal,
                error.message
            };
    }

    return submission_event_error::internal;
}

const submission_event_error submission_event_error::invalid_argument{
    submission_event_error_code::invalid_argument
};
const submission_event_error submission_event_error::unavailable{
    submission_event_error_code::unavailable
};
const submission_event_error submission_event_error::internal{
    submission_event_error_code::internal
};

std::string to_string(submission_event_error_code error){
    return std::string{describe_submission_event_error(error).default_message};
}

std::string to_string(const submission_event_error& error){
    return error.message;
}
