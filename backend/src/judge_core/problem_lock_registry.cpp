#include "judge_core/problem_lock_registry.hpp"

#include <utility>

problem_lock_registry::scoped_lock::scoped_lock(
    std::shared_ptr<entry> entry_value,
    std::unique_lock<std::mutex> lock_value
) :
    entry_(std::move(entry_value)),
    lock_(std::move(lock_value)){}

std::expected<problem_lock_registry::scoped_lock, error_code> problem_lock_registry::lock(
    std::int64_t problem_id
){
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    std::shared_ptr<entry> entry_value;
    {
        std::scoped_lock registry_lock(registry_mutex_);
        auto entry_it = entries_.find(problem_id);
        if(entry_it != entries_.end()){
            entry_value = entry_it->second.lock();
        }

        if(!entry_value){
            entry_value = std::make_shared<entry>();
            entries_[problem_id] = entry_value;
        }
    }

    std::unique_lock<std::mutex> problem_lock(entry_value->mutex_);
    return scoped_lock(std::move(entry_value), std::move(problem_lock));
}
