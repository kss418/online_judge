#include "error/transport_error.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/error.hpp>

#include <utility>

namespace{
    std::string default_message(transport_error_code error){
        switch(error){
            case transport_error_code::invalid_argument:
                return "invalid transport argument";
            case transport_error_code::bad_descriptor:
                return "bad transport descriptor";
            case transport_error_code::unavailable:
                return "transport unavailable";
            case transport_error_code::internal:
                return "transport internal error";
        }

        return "unknown transport error";
    }
}

transport_error::transport_error(
    transport_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

transport_error::transport_error(const boost::system::error_code& error)
:
    transport_error(from_boost_error(error)){}

bool transport_error::operator==(const transport_error& other) const{
    return code == other.code;
}

const transport_error transport_error::invalid_argument{
    transport_error_code::invalid_argument
};
const transport_error transport_error::bad_descriptor{
    transport_error_code::bad_descriptor
};
const transport_error transport_error::unavailable{
    transport_error_code::unavailable
};
const transport_error transport_error::internal{
    transport_error_code::internal
};

std::string to_string(transport_error_code error){
    switch(error){
        case transport_error_code::invalid_argument:
            return "invalid transport argument";
        case transport_error_code::bad_descriptor:
            return "bad transport descriptor";
        case transport_error_code::unavailable:
            return "transport unavailable";
        case transport_error_code::internal:
            return "transport internal error";
    }

    return "unknown transport error";
}

std::string to_string(const transport_error& error){
    return error.message;
}

transport_error transport_error::from_boost_error(const boost::system::error_code& error){
    if(
        error == boost::asio::error::bad_descriptor ||
        error == boost::asio::error::already_open ||
        error == boost::asio::error::not_connected
    ){
        return transport_error{
            transport_error_code::bad_descriptor,
            error.message()
        };
    }

    if(
        error == boost::asio::error::operation_aborted ||
        error == boost::asio::error::timed_out ||
        error == boost::asio::error::would_block ||
        error == boost::asio::error::try_again ||
        error == boost::asio::error::connection_refused ||
        error == boost::asio::error::connection_reset ||
        error == boost::asio::error::connection_aborted ||
        error == boost::asio::error::eof ||
        error == boost::beast::http::error::end_of_stream
    ){
        return transport_error{
            transport_error_code::unavailable,
            error.message()
        };
    }

    return transport_error{
        transport_error_code::internal,
        error.message()
    };
}
