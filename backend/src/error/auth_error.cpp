#include "error/auth_error.hpp"

namespace auth_error{
    http_error missing_or_invalid_bearer_token(){
        return http_error{http_error_code::missing_or_invalid_bearer_token};
    }

    http_error invalid_or_expired_token(){
        return http_error{http_error_code::invalid_or_expired_token};
    }

    http_error admin_bearer_token_required(){
        return http_error{http_error_code::admin_bearer_token_required};
    }

    http_error superadmin_bearer_token_required(){
        return http_error{http_error_code::superadmin_bearer_token_required};
    }

    http_error invalid_credentials(){
        return http_error{http_error_code::invalid_credentials};
    }

    std::optional<http_error> map_login_service_error(
        const service_error& error
    ){
        if(error == service_error::unauthorized){
            return invalid_credentials();
        }

        return std::nullopt;
    }

    std::optional<http_error> map_token_service_error(
        const service_error& error
    ){
        if(error == service_error::unauthorized){
            return invalid_or_expired_token();
        }

        return std::nullopt;
    }
}
