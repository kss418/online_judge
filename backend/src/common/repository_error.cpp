#include "common/repository_error.hpp"

std::string to_string(repository_error ec){
    switch(ec){
        case repository_error::invalid_reference:
            return "invalid reference";
        case repository_error::invalid_input:
            return "invalid input";
        case repository_error::not_found:
            return "repository not found";
        case repository_error::conflict:
            return "repository conflict";
    }
    return "unknown repository error";
}
