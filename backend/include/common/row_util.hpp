#pragma once

#include <pqxx/pqxx>

#include <cstddef>
#include <optional>

namespace row_util{
    template <typename T>
    T get_required(const pqxx::row& row, std::size_t column_index){
        return row[column_index].as<T>();
    }

    template <typename T>
    std::optional<T> get_optional(const pqxx::row& row, std::size_t column_index){
        if(row[column_index].is_null()){
            return std::nullopt;
        }

        return row[column_index].as<T>();
    }
}
