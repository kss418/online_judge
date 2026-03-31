#include "common/db_connection_pool.hpp"

#include <condition_variable>
#include <deque>
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
    std::deque<std::size_t> free_slot_indices_;

    std::expected<std::size_t, error_code> acquire_slot_index(
        std::chrono::milliseconds timeout,
        bool use_timeout
    ){
        std::unique_lock lock(mutex_);
        const auto has_available_slot = [&]{
            return !free_slot_indices_.empty();
        };

        if(use_timeout){
            if(!condition_variable_.wait_for(lock, timeout, has_available_slot)){
                return std::unexpected(error_code::create(boost_error::timed_out));
            }
        }
        else{
            condition_variable_.wait(lock, has_available_slot);
        }

        const std::size_t slot_index = free_slot_indices_.front();
        free_slot_indices_.pop_front();
        slots_[slot_index].in_use_ = true;
        return slot_index;
    }

    void release_slot_index(std::size_t slot_index){
        std::scoped_lock lock(mutex_);
        if(slot_index >= slots_.size() || !slots_[slot_index].in_use_){
            return;
        }

        slots_[slot_index].in_use_ = false;
        free_slot_indices_.push_back(slot_index);
        condition_variable_.notify_one();
    }

    std::size_t available_count() const{
        std::scoped_lock lock(mutex_);
        return free_slot_indices_.size();
    }
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
    return state_ != nullptr;
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

    state_->release_slot_index(slot_index_);
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
        state_value->free_slot_indices_.push_back(index);
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

    return state_->available_count();
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

    const auto slot_index_exp = state_->acquire_slot_index(timeout, use_timeout);
    if(!slot_index_exp){
        return std::unexpected(slot_index_exp.error());
    }
    const std::size_t slot_index = *slot_index_exp;

    db_connection& connection_value = state_->slots_[slot_index].connection_;
    if(!connection_value.is_connected()){
        const auto reconnect_exp = connection_value.reconnect();
        if(!reconnect_exp){
            state_->release_slot_index(slot_index);
            return std::unexpected(reconnect_exp.error());
        }
    }

    return lease(state_, slot_index);
}
