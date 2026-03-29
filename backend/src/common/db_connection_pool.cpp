#include "common/db_connection_pool.hpp"

#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

struct db_connection_pool::state{
    struct slot{
        db_connection connection_;
        bool in_use_ = false;
    };

    mutable std::mutex mutex_;
    std::condition_variable condition_variable_;
    std::vector<slot> slots_;
};

db_connection_pool::lease::lease(
    std::shared_ptr<state> state_value,
    std::size_t slot_index
) :
    state_(std::move(state_value)),
    slot_index_(slot_index){}

db_connection_pool::lease::lease(lease&& other) noexcept :
    state_(std::move(other.state_)),
    slot_index_(other.slot_index_){
    other.slot_index_ = 0;
}

db_connection_pool::lease& db_connection_pool::lease::operator=(lease&& other) noexcept{
    if(this == &other){
        return *this;
    }

    release();
    state_ = std::move(other.state_);
    slot_index_ = other.slot_index_;
    other.slot_index_ = 0;
    return *this;
}

db_connection_pool::lease::~lease(){
    release();
}

db_connection_pool::lease::operator bool() const{
    return
        state_ &&
        slot_index_ < state_->slots_.size() &&
        state_->slots_[slot_index_].in_use_;
}

db_connection& db_connection_pool::lease::connection(){
    return state_->slots_[slot_index_].connection_;
}

const db_connection& db_connection_pool::lease::connection() const{
    return state_->slots_[slot_index_].connection_;
}

db_connection& db_connection_pool::lease::operator*(){
    return connection();
}

const db_connection& db_connection_pool::lease::operator*() const{
    return connection();
}

db_connection* db_connection_pool::lease::operator->(){
    return &connection();
}

const db_connection* db_connection_pool::lease::operator->() const{
    return &connection();
}

void db_connection_pool::lease::release(){
    if(!state_){
        return;
    }

    const auto release_slot_locked = [&]{
        if(slot_index_ >= state_->slots_.size()){
            return;
        }

        state_->slots_[slot_index_].in_use_ = false;
        state_->condition_variable_.notify_one();
    };

    {
        std::scoped_lock lock(state_->mutex_);
        release_slot_locked();
    }

    state_.reset();
    slot_index_ = 0;
}

std::expected<db_connection_pool, error_code> db_connection_pool::create(std::size_t pool_size){
    if(pool_size == 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    auto state_value = std::make_shared<state>();
    state_value->slots_.reserve(pool_size);

    for(std::size_t index = 0; index < pool_size; ++index){
        auto connection_exp = db_connection::create();
        if(!connection_exp){
            return std::unexpected(connection_exp.error());
        }

        state::slot slot_value;
        slot_value.connection_ = std::move(*connection_exp);
        state_value->slots_.push_back(std::move(slot_value));
    }

    return db_connection_pool(std::move(state_value));
}

std::expected<db_connection_pool::lease, error_code> db_connection_pool::acquire(){
    return acquire_impl(std::chrono::milliseconds::zero(), false);
}

std::expected<db_connection_pool::lease, error_code> db_connection_pool::acquire_for(
    std::chrono::milliseconds timeout
){
    if(timeout < std::chrono::milliseconds::zero()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return acquire_impl(timeout, true);
}

std::size_t db_connection_pool::size() const{
    return state_ ? state_->slots_.size() : 0;
}

std::size_t db_connection_pool::available_count() const{
    if(!state_){
        return 0;
    }

    std::scoped_lock lock(state_->mutex_);
    std::size_t available_slot_count = 0;
    for(const auto& slot_value : state_->slots_){
        if(!slot_value.in_use_){
            ++available_slot_count;
        }
    }

    return available_slot_count;
}

db_connection_pool::db_connection_pool(std::shared_ptr<state> state_value) :
    state_(std::move(state_value)){}

std::expected<db_connection_pool::lease, error_code> db_connection_pool::acquire_impl(
    std::chrono::milliseconds timeout,
    bool use_timeout
){
    if(!state_){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    std::size_t slot_index = 0;
    {
        std::unique_lock lock(state_->mutex_);
        const auto find_available_slot_locked = [&]{
            for(std::size_t index = 0; index < state_->slots_.size(); ++index){
                if(!state_->slots_[index].in_use_){
                    return index;
                }
            }

            return state_->slots_.size();
        };
        const auto wait_predicate = [&]{
            return find_available_slot_locked() < state_->slots_.size();
        };

        if(use_timeout){
            const bool has_available_slot = state_->condition_variable_.wait_for(
                lock,
                timeout,
                wait_predicate
            );
            if(!has_available_slot){
                return std::unexpected(error_code::create(boost_error::timed_out));
            }
        }
        else{
            state_->condition_variable_.wait(lock, wait_predicate);
        }

        slot_index = find_available_slot_locked();
        state_->slots_[slot_index].in_use_ = true;
    }

    db_connection& connection_value = state_->slots_[slot_index].connection_;
    if(!connection_value.is_connected()){
        const auto reconnect_exp = connection_value.reconnect();
        if(!reconnect_exp){
            std::scoped_lock lock(state_->mutex_);
            if(slot_index < state_->slots_.size()){
                state_->slots_[slot_index].in_use_ = false;
                state_->condition_variable_.notify_one();
            }
            return std::unexpected(reconnect_exp.error());
        }
    }

    return lease(state_, slot_index);
}
