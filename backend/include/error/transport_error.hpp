#pragma once

#include <boost/system/error_code.hpp>

#include <string>

enum class transport_error_code{
    invalid_argument,
    bad_descriptor,
    unavailable,
    internal
};

struct transport_error{
    transport_error_code code;
    std::string message;

    transport_error(transport_error_code code_value, std::string message_value = {});
    transport_error(const boost::system::error_code& error);

    bool operator==(const transport_error& other) const;

    static const transport_error invalid_argument;
    static const transport_error bad_descriptor;
    static const transport_error unavailable;
    static const transport_error internal;

    static transport_error from_boost_error(const boost::system::error_code& error);
};

std::string to_string(transport_error_code error);
std::string to_string(const transport_error& error);
