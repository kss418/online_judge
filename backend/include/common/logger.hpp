#pragma once

#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

class logger{
public:
    class record{
    public:
        record(const record&) = delete;
        record& operator=(const record&) = delete;
        record(record&& other) noexcept;
        record& operator=(record&& other) = delete;
        ~record() noexcept;

        template <typename value_type>
        record& field(std::string_view key, const value_type& value){
            append_key(key);
            stream_ << value;
            return *this;
        }

        record& field(std::string_view key, std::string_view value);
        record& field(std::string_view key, const std::string& value);
        record& field(std::string_view key, const char* value);

        template <typename value_type>
        record& optional_field(
            std::string_view key,
            const std::optional<value_type>& value_opt
        ){
            if(value_opt.has_value()){
                field(key, *value_opt);
            }

            return *this;
        }

        void flush() noexcept;

    private:
        friend class logger;

        explicit record(logger& owner, std::string_view event);

        void append_key(std::string_view key);
        static void append_encoded_text(
            std::ostringstream& stream,
            std::string_view value
        );

        logger* owner_ = nullptr;
        std::ostringstream stream_;
        bool needs_separator_ = false;
        bool is_flushed_ = false;
    };

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) noexcept = delete;
    logger& operator=(logger&&) noexcept = delete;

    explicit logger(std::ostream& output) noexcept;

    record log(std::string_view event);

    static logger& clog();
    static logger& cerr();

private:
    friend class record;

    void write(std::string message) noexcept;

    std::ostream* output_ = nullptr;
    std::mutex mutex_;
};
