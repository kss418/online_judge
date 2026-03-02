#include "http_server/http_server.hpp"

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
    auto http_server_exp = http_server::create(
        io_context,
        http_server::tcp::endpoint{http_server::tcp::v4(), port}
    );
    
    if(!http_server_exp){
        std::cerr << "http_server create failed: " << to_string(http_server_exp.error()) << '\n';
        return 1;
    }

    auto run_exp = (*http_server_exp)->run();
    if(!run_exp){
        std::cerr << "http_server run failed: " << to_string(run_exp.error()) << '\n';
        return 1;
    }

    io_context.run();
    return 0;
}
