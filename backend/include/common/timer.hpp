#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace timer_detail{
    template <typename callback_type>
    class scope_exit{
    public:
        explicit scope_exit(callback_type&& callback) :
            callback_(std::forward<callback_type>(callback)){}

        scope_exit(const scope_exit&) = delete;
        scope_exit& operator=(const scope_exit&) = delete;

        scope_exit(scope_exit&& other) noexcept(
            std::is_nothrow_move_constructible_v<callback_type>
        ) :
            callback_(std::move(other.callback_)),
            is_active_(std::exchange(other.is_active_, false)){}

        ~scope_exit(){
            if(is_active_){
                callback_();
            }
        }

    private:
        callback_type callback_;
        bool is_active_ = true;
    };

    template <typename callback_type>
    auto make_scope_exit(callback_type&& callback){
        return scope_exit<std::decay_t<callback_type>>(
            std::forward<callback_type>(callback)
        );
    }
}

class timer{
public:
    using clock_type = std::chrono::steady_clock;

    timer() : started_at_(clock_type::now()){}

    void reset(){
        started_at_ = clock_type::now();
    }

    std::int64_t elapsed_ms() const{
        return elapsed_ms_between(started_at_, clock_type::now());
    }

    static std::int64_t elapsed_ms_between(
        const clock_type::time_point& started_at,
        const clock_type::time_point& finished_at
    ){
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            finished_at - started_at
        ).count();
    }

    template <typename fn_type, typename... arg_types>
    static decltype(auto) measure_elapsed_ms(
        std::int64_t& elapsed_ms_out,
        fn_type&& fn,
        arg_types&&... args
    ){
        timer timer_value;
        auto record_elapsed = timer_detail::make_scope_exit([&elapsed_ms_out, &timer_value]{
            elapsed_ms_out = timer_value.elapsed_ms();
        });

        return std::invoke(
            std::forward<fn_type>(fn),
            std::forward<arg_types>(args)...
        );
    }

private:
    clock_type::time_point started_at_;
};
