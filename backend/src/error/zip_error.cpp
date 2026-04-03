#include "error/zip_error.hpp"

#include "error/io_error.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace{
    struct zip_error_spec{
        std::string_view default_message;
    };

    constexpr zip_error_spec unknown_zip_error_spec{
        "unknown zip error"
    };

    constexpr std::array<zip_error_spec, 5> zip_error_specs{{
        {"invalid zip argument"},
        {"zip command unavailable"},
        {"invalid zip archive"},
        {"zip unavailable"},
        {"zip internal error"},
    }};

    const zip_error_spec& describe_zip_error(zip_error_code error){
        const auto index = static_cast<std::size_t>(error);
        if(index >= zip_error_specs.size()){
            return unknown_zip_error_spec;
        }

        return zip_error_specs[index];
    }
}

zip_error::zip_error(
    zip_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_zip_error(code_value).default_message}
            : std::move(message_value)
    ){}

zip_error::zip_error(const io_error& error)
:
    zip_error(from_io_error(error)){}

bool zip_error::operator==(const zip_error& other) const{
    return code == other.code;
}

zip_error zip_error::from_io_error(const io_error& error){
    switch(error.code){
        case io_error_code::unavailable:
            return zip_error{
                zip_error_code::unavailable,
                error.message
            };
        case io_error_code::invalid_argument:
        case io_error_code::permission_denied:
        case io_error_code::not_found:
        case io_error_code::conflict:
        case io_error_code::internal:
            return zip_error{
                zip_error_code::internal,
                error.message
            };
    }

    return zip_error::internal;
}

zip_error zip_error::from_errno(int error_number){
    return from_io_error(io_error::from_errno(error_number));
}

zip_error zip_error::from_error_code(const std::error_code& error){
    return from_io_error(io_error::from_error_code(error));
}

const zip_error zip_error::invalid_argument{
    zip_error_code::invalid_argument
};
const zip_error zip_error::command_unavailable{
    zip_error_code::command_unavailable
};
const zip_error zip_error::invalid_archive{
    zip_error_code::invalid_archive
};
const zip_error zip_error::unavailable{
    zip_error_code::unavailable
};
const zip_error zip_error::internal{
    zip_error_code::internal
};

std::string to_string(zip_error_code error){
    return std::string{describe_zip_error(error).default_message};
}

std::string to_string(const zip_error& error){
    return error.message;
}
