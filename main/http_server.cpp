#include "http_server/net/acceptor.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <charconv>
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

int main(int argc, char** argv){
    std::uint16_t port = 8080;
    if(argc >= 2){
        auto port_exp = parse_port(argv[1]);
        if(!port_exp){
            std::cerr << "invalid port: " << argv[1] << '\n';
            return 1;
        }
        port = *port_exp;
    }

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
