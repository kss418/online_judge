#pragma once

#include <pqxx/pqxx>

#include <string>

struct assembled_query{
    std::string sql;
    pqxx::params params;
};
