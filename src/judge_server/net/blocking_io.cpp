#include "judge_server/net/blocking_io.hpp"

#include <unistd.h>

std::expected <size_t, error_code> blocking_io::write_all(int fd, const std::string& input_text){
    size_t total = 0;
    while(total < input_text.size()){
        ssize_t write_bytes = write(fd, input_text.data() + total, input_text.size() - total);
        if(write_bytes > 0){
            total += static_cast<size_t>(write_bytes);
            continue;
        }

        if(write_bytes < 0 && errno == EINTR) continue;
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }
    return total;
}

std::expected<std::string, error_code> blocking_io::read_all(int fd){
    std::string output;
    char buf[4096];

    while(true){
        ssize_t n = read(fd, buf, sizeof(buf));
        if(n > 0){
            output.append(buf, static_cast<size_t>(n));
            continue;
        }

        if(n == 0) break;
        if(errno == EINTR) continue;
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }

    return output;
}

std::expected<std::string, error_code> blocking_io::read_all_from_start(int fd){
    if(::lseek(fd, 0, SEEK_SET) < 0){
        return std::unexpected(error_code::create(error_code::map_errno(errno)));
    }
    return blocking_io::read_all(fd);
}
