#include "http_server/acceptor.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string_view>

std::expected<std::uint16_t, error_code> parse_port(std::string_view text){
    std::uint16_t port = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    auto parse_result = std::from_chars(begin, end, port);
    if(parse_result.ec != std::errc() || parse_result.ptr != end || port == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return port;
}

int main(){
    const char* http_port_text = std::getenv("HTTP_PORT");
    if(http_port_text == nullptr || std::string_view{http_port_text}.empty()){
        std::cerr << "HTTP_PORT environment variable is missing\n";
        return 1;
    }

    auto port_exp = parse_port(http_port_text);
    if(!port_exp){
        std::cerr << "invalid HTTP_PORT: " << http_port_text << '\n';
        return 1;
    }
    std::uint16_t port = *port_exp;

    boost::asio::io_context io_context{1};
    auto acceptor_exp = acceptor::create(
        io_context,
        acceptor::tcp::endpoint{acceptor::tcp::v4(), port}
    );
    
    if(!acceptor_exp){
        std::cerr << "acceptor create failed: " << to_string(acceptor_exp.error()) << '\n';
        return 1;
    }

    auto run_exp = (*acceptor_exp)->run();
    if(!run_exp){
        std::cerr << "acceptor run failed: " << to_string(run_exp.error()) << '\n';
        return 1;
    }

    io_context.run();
    return 0;
}
