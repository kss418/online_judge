#include "common/logger.hpp"

#include <cctype>
#include <iostream>

namespace{
    constexpr char HEX_DIGITS[] = "0123456789ABCDEF";
}

logger::record::record(logger& owner, std::string_view event)
    : owner_(&owner){
    if(event.empty()){
        return;
    }

    stream_ << event;
    needs_separator_ = true;
}

logger::record::record(record&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)),
      stream_(std::move(other.stream_)),
      needs_separator_(other.needs_separator_),
      is_flushed_(other.is_flushed_){
    other.is_flushed_ = true;
}

logger::record::~record() noexcept{
    flush();
}

logger::record& logger::record::field(
    std::string_view key,
    std::string_view value
){
    append_key(key);
    append_encoded_text(stream_, value);
    return *this;
}

logger::record& logger::record::field(
    std::string_view key,
    const std::string& value
){
    return field(key, std::string_view(value));
}

logger::record& logger::record::field(
    std::string_view key,
    const char* value
){
    return field(
        key,
        value == nullptr ? std::string_view{} : std::string_view(value)
    );
}

void logger::record::flush() noexcept{
    if(owner_ == nullptr || is_flushed_){
        return;
    }

    try{
        owner_->write(stream_.str());
    }
    catch(...){
    }

    owner_ = nullptr;
    is_flushed_ = true;
}

void logger::record::append_key(std::string_view key){
    if(needs_separator_){
        stream_ << ' ';
    }

    stream_ << key << '=';
    needs_separator_ = true;
}

void logger::record::append_encoded_text(
    std::ostringstream& stream,
    std::string_view value
){
    for(const char ch : value){
        const auto unsigned_ch = static_cast<unsigned char>(ch);
        if(
            std::isspace(unsigned_ch) != 0 ||
            ch == '\\' ||
            ch == '=' ||
            !std::isprint(unsigned_ch)
        ){
            stream
                << "\\x"
                << HEX_DIGITS[(unsigned_ch >> 4U) & 0x0FU]
                << HEX_DIGITS[unsigned_ch & 0x0FU];
            continue;
        }

        stream << ch;
    }
}

logger::logger(std::ostream& output) noexcept
    : output_(&output){}

logger::record logger::log(std::string_view event){
    return record(*this, event);
}

logger& logger::clog(){
    static logger clog_logger(std::clog);
    return clog_logger;
}

logger& logger::cerr(){
    static logger cerr_logger(std::cerr);
    return cerr_logger;
}

void logger::write(std::string message) noexcept{
    std::lock_guard<std::mutex> lock(mutex_);
    (*output_) << message << '\n' << std::flush;
}
