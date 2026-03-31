#pragma once

#include <pqxx/pqxx>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class sql_filter_builder{
public:
    void where(std::string clause){
        clauses_.push_back(std::move(clause));
    }

    template <typename value_type>
    void where_param(std::string prefix, value_type&& value){
        where(std::move(prefix) + append_param(std::forward<value_type>(value)));
    }

    template <typename value_type>
    void where_param_with_suffix(
        std::string prefix,
        value_type&& value,
        std::string suffix
    ){
        where(
            std::move(prefix) +
            append_param(std::forward<value_type>(value)) +
            std::move(suffix)
        );
    }

    template <typename value_type>
    void where_optional_param(
        std::string prefix,
        const std::optional<value_type>& value_opt
    ){
        if(value_opt){
            where_param(std::move(prefix), *value_opt);
        }
    }

    template <typename value_type>
    void where_optional_param_with_suffix(
        std::string prefix,
        const std::optional<value_type>& value_opt,
        std::string suffix
    ){
        if(value_opt){
            where_param_with_suffix(std::move(prefix), *value_opt, std::move(suffix));
        }
    }

    template <typename value_type>
    std::string append_param(value_type&& value){
        const std::string placeholder = "$" + std::to_string(next_param_index_);
        ++next_param_index_;
        params_.append(std::forward<value_type>(value));
        return placeholder;
    }

    std::string sql(std::string_view prefix = "WHERE 1 = 1") const{
        std::string sql_value{prefix};
        for(const auto& clause : clauses_){
            sql_value += " AND ";
            sql_value += clause;
        }
        return sql_value;
    }

    const pqxx::params& params() const{
        return params_;
    }

    pqxx::params&& take_params(){
        return std::move(params_);
    }

    int next_param_index() const{
        return next_param_index_;
    }

private:
    std::vector<std::string> clauses_;
    pqxx::params params_;
    int next_param_index_ = 1;
};
