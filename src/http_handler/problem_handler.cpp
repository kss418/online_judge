#include "http_handler/problem_handler.hpp"

problem_handler::problem_handler(db_connection& db_connection) :
    db_connection_(db_connection){}

bool problem_handler::is_problem_path(std::string_view path){
    return path.starts_with(path_prefix_);
}
