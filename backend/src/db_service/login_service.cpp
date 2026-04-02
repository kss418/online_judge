#include "db_service/login_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_service/service_error_bridge.hpp"
#include "common/token_util.hpp"
#include "db_repository/auth_repository.hpp"
#include "db_repository/login_repository.hpp"
#include "common/permission_util.hpp"

std::expected<auth_dto::session, service_error> login_service::sign_up(
    db_connection& connection_value,
    const auth_dto::sign_up_request& sign_up_request_value
){
    const auto hashed_sign_up_request_exp = auth_dto::hash_sign_up_request(
        sign_up_request_value
    );
    if(!hashed_sign_up_request_exp){
        return std::unexpected(
            db_service_util::map_error_to_service_error(
                hashed_sign_up_request_exp.error()
            )
        );
    }

    const auto issued_token_exp = token_util::issue_token();
    if(!issued_token_exp){
        return std::unexpected(
            db_service_util::map_error_to_service_error(issued_token_exp.error())
        );
    }
    auth_dto::hashed_token hashed_token_value;
    hashed_token_value.token_hash = issued_token_exp->token_hash;

    return db_service_util::map_error_to_service_error(
        db_service_util::with_retry_write_transaction(
            connection_value,
            [&](pqxx::work& transaction) -> std::expected<auth_dto::session, error_code> {
                const auto user_id_exp = login_repository::create_user(
                    transaction,
                    *hashed_sign_up_request_exp
                );
                if(!user_id_exp){
                    return std::unexpected(
                        error_code::create(
                            db_service_error_bridge::map_repository_error(
                                user_id_exp.error()
                            )
                        )
                    );
                }

                const auto insert_token_exp = auth_repository::insert_token(
                    transaction,
                    *user_id_exp,
                    hashed_token_value,
                    token_util::TOKEN_TTL
                );
                if(!insert_token_exp){
                    return std::unexpected(
                        error_code::create(
                            db_service_error_bridge::map_repository_error(
                                insert_token_exp.error()
                            )
                        )
                    );
                }

                auth_dto::session session_value;
                session_value.user_id = *user_id_exp;
                session_value.permission_level = permission_util::USER;
                session_value.user_login_id = hashed_sign_up_request_exp->user_login_id;
                session_value.token = issued_token_exp->token;
                return session_value;
            }
        )
    );
}

std::expected<std::optional<auth_dto::session>, service_error> login_service::login(
    db_connection& connection_value,
    const auth_dto::credentials& credentials_value
){
    const auto hashed_credentials_exp = auth_dto::hash_credentials(
        credentials_value
    );
    if(!hashed_credentials_exp){
        return std::unexpected(
            db_service_util::map_error_to_service_error(hashed_credentials_exp.error())
        );
    }

    return db_service_util::map_error_to_service_error(
        db_service_util::with_retry_write_transaction(
            connection_value,
            [&](pqxx::work& transaction)
                -> std::expected<std::optional<auth_dto::session>, error_code> {
                const auto login_identity_exp = login_repository::get_login_identity(
                    transaction,
                    *hashed_credentials_exp
                );
                if(!login_identity_exp){
                    return std::unexpected(
                        error_code::create(
                            db_service_error_bridge::map_repository_error(
                                login_identity_exp.error()
                            )
                        )
                    );
                }
                if(!login_identity_exp->has_value()){
                    return std::nullopt;
                }

                const auto issued_token_exp = token_util::issue_token();
                if(!issued_token_exp){
                    return std::unexpected(issued_token_exp.error());
                }
                auth_dto::hashed_token hashed_token_value;
                hashed_token_value.token_hash = issued_token_exp->token_hash;

                const auto insert_token_exp = auth_repository::insert_token(
                    transaction,
                    login_identity_exp->value().user_id,
                    hashed_token_value,
                    token_util::TOKEN_TTL
                );
                if(!insert_token_exp){
                    return std::unexpected(
                        error_code::create(
                            db_service_error_bridge::map_repository_error(
                                insert_token_exp.error()
                            )
                        )
                    );
                }

                auth_dto::session session_value;
                session_value.user_id = login_identity_exp->value().user_id;
                session_value.permission_level = login_identity_exp->value().permission_level;
                session_value.user_login_id = login_identity_exp->value().user_login_id;
                session_value.token = issued_token_exp->token;
                return session_value;
            }
        )
    );
}
