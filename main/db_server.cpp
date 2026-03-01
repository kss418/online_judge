#include <pqxx/pqxx>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

bool is_empty_env(const char* value){
    return value == nullptr || std::string_view{value}.empty();
}

std::string create_connection_string(){
    const char* db_user = std::getenv("DB_USER");
    const char* db_password = std::getenv("DB_PASSWORD");
    const char* db_host = std::getenv("DB_HOST");
    const char* db_name = std::getenv("DB_NAME");
    const char* db_port = std::getenv("DB_PORT");

    if(
        is_empty_env(db_user) ||
        is_empty_env(db_password) ||
        is_empty_env(db_host) ||
        is_empty_env(db_name) || 
        is_empty_env(db_port)
    ){
        return {};
    }

    return
        "postgresql://" +
        std::string{db_user} +
        ":" +
        std::string{db_password} +
        "@" +
        std::string{db_host} +
        ":" +
        std::string{db_port} +
        "/" +
        std::string{db_name};
}

int main(){
    const std::string connection_string = create_connection_string();
    if(connection_string.empty()){
        std::cerr << "set DB_USER/DB_PASSWORD/DB_HOST/DB_PORT/DB_NAME environment variables\n";
        return 1;
    }

    try{
        pqxx::connection connection{connection_string};
        if(!connection.is_open()){
            std::cerr << "db_server error: failed to open connection\n";
            return 1;
        }

        pqxx::read_transaction read_tx{connection};
        const pqxx::result result = read_tx.exec("select 1 as ok");
        if(result.size() != 1 || result.columns() != 1 || result[0][0].as<int>(0) != 1){
            std::cerr << "db_server error: health query failed\n";
            return 1;
        }

        std::cout << "db_server connected: " << connection.dbname() << '\n';
        return 0;
    }
    catch(const std::exception& exception){
        std::cerr << "db_server error: " << exception.what() << '\n';
        return 1;
    }
}
