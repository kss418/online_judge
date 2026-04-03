#include "error/sandbox_error.hpp"

#include "error/infra_error.hpp"
#include "error/io_error.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct sandbox_error_spec{
        std::string_view default_message;
    };

    constexpr sandbox_error_spec unknown_sandbox_error_spec{
        "unknown sandbox error"
    };

    constexpr std::array<sandbox_error_spec, 5> sandbox_error_specs{{
        {"invalid sandbox argument"},
        {"invalid sandbox configuration"},
        {"sandbox unsupported"},
        {"sandbox unavailable"},
        {"sandbox internal error"},
    }};

    const sandbox_error_spec& describe_sandbox_error(sandbox_error_code error){
        const auto index = static_cast<std::size_t>(error);
        if(index >= sandbox_error_specs.size()){
            return unknown_sandbox_error_spec;
        }

        return sandbox_error_specs[index];
    }
}

sandbox_error::sandbox_error(
    sandbox_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_sandbox_error(code_value).default_message}
            : std::move(message_value)
    ){}

sandbox_error::sandbox_error(const io_error& error)
:
    sandbox_error(from_io_error(error)){}

sandbox_error::sandbox_error(const infra_error& error)
:
    sandbox_error(from_infra_error(error)){}

bool sandbox_error::operator==(const sandbox_error& other) const{
    return code == other.code;
}

sandbox_error sandbox_error::from_io_error(const io_error& error){
    switch(error.code){
        case io_error_code::invalid_argument:
            return sandbox_error{
                sandbox_error_code::invalid_argument,
                error.message
            };
        case io_error_code::unavailable:
            return sandbox_error{
                sandbox_error_code::unavailable,
                error.message
            };
        case io_error_code::permission_denied:
        case io_error_code::not_found:
        case io_error_code::conflict:
        case io_error_code::internal:
            return sandbox_error{
                sandbox_error_code::internal,
                error.message
            };
    }

    return sandbox_error::internal;
}

sandbox_error sandbox_error::from_infra_error(const infra_error& error){
    switch(error.code){
        case infra_error_code::invalid_argument:
        case infra_error_code::permission_denied:
        case infra_error_code::not_found:
        case infra_error_code::conflict:
            return sandbox_error{
                sandbox_error_code::invalid_configuration,
                error.message
            };
        case infra_error_code::unavailable:
            return sandbox_error{
                sandbox_error_code::unavailable,
                error.message
            };
        case infra_error_code::internal:
            return sandbox_error{
                sandbox_error_code::internal,
                error.message
            };
    }

    return sandbox_error::internal;
}

sandbox_error sandbox_error::from_errno(int error_number){
    return from_io_error(io_error::from_errno(error_number));
}

sandbox_error sandbox_error::from_error_code(const std::error_code& error){
    return from_io_error(io_error::from_error_code(error));
}

const sandbox_error sandbox_error::invalid_argument{
    sandbox_error_code::invalid_argument
};
const sandbox_error sandbox_error::invalid_configuration{
    sandbox_error_code::invalid_configuration
};
const sandbox_error sandbox_error::unsupported{
    sandbox_error_code::unsupported
};
const sandbox_error sandbox_error::unavailable{
    sandbox_error_code::unavailable
};
const sandbox_error sandbox_error::internal{
    sandbox_error_code::internal
};

std::string to_string(sandbox_error_code error){
    return std::string{describe_sandbox_error(error).default_message};
}

std::string to_string(const sandbox_error& error){
    return error.message;
}
