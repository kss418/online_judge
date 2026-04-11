#include "error/http_error.hpp"

#include <array>
#include <span>

namespace{
    constexpr std::array<http_error_spec, 26> http_error_specs{{
        http_error_spec{
            http_error_code::bad_request,
            "bad_request",
            boost::beast::http::status::bad_request,
            "bad request",
            "요청 형식이 올바르지 않습니다.",
            "",
            false,
            false,
            "protocol"
        },
        http_error_spec{
            http_error_code::payload_too_large,
            "payload_too_large",
            boost::beast::http::status::payload_too_large,
            "request body too large",
            "요청 본문이 너무 큽니다.",
            "",
            false,
            false,
            "protocol"
        },
        http_error_spec{
            http_error_code::method_not_allowed,
            "method_not_allowed",
            boost::beast::http::status::method_not_allowed,
            "method not allowed",
            "지원되지 않는 요청 방식입니다.",
            "",
            false,
            false,
            "protocol"
        },
        http_error_spec{
            http_error_code::validation_error,
            "validation_error",
            boost::beast::http::status::bad_request,
            "validation error",
            "입력값을 다시 확인해 주세요.",
            "",
            false,
            false,
            "validation"
        },
        http_error_spec{
            http_error_code::unauthorized,
            "unauthorized",
            boost::beast::http::status::unauthorized,
            "unauthorized",
            "로그인이 필요합니다.",
            "",
            false,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::forbidden,
            "forbidden",
            boost::beast::http::status::forbidden,
            "forbidden",
            "권한이 없습니다.",
            "",
            false,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::not_found,
            "not_found",
            boost::beast::http::status::not_found,
            "not found",
            "요청한 대상을 찾지 못했습니다.",
            "",
            false,
            false,
            "resource"
        },
        http_error_spec{
            http_error_code::conflict,
            "conflict",
            boost::beast::http::status::conflict,
            "conflict",
            "이미 존재하거나 현재 상태와 충돌합니다.",
            "",
            false,
            false,
            "state"
        },
        http_error_spec{
            http_error_code::service_unavailable,
            "service_unavailable",
            boost::beast::http::status::service_unavailable,
            "service unavailable",
            "서비스가 일시적으로 불안정합니다. 잠시 후 다시 시도해 주세요.",
            "",
            false,
            false,
            "availability"
        },
        http_error_spec{
            http_error_code::internal_server_error,
            "internal_server_error",
            boost::beast::http::status::internal_server_error,
            "internal server error",
            "서버에서 요청을 처리하지 못했습니다. 잠시 후 다시 시도해 주세요.",
            "",
            false,
            false,
            "server"
        },
        http_error_spec{
            http_error_code::invalid_json,
            "invalid_json",
            boost::beast::http::status::bad_request,
            "invalid json",
            "요청 본문 JSON 형식이 올바르지 않습니다.",
            "",
            false,
            false,
            "request"
        },
        http_error_spec{
            http_error_code::invalid_query_string,
            "invalid_query_string",
            boost::beast::http::status::bad_request,
            "invalid query string",
            "쿼리 문자열 형식이 올바르지 않습니다.",
            "",
            false,
            false,
            "request"
        },
        http_error_spec{
            http_error_code::duplicate_query_parameter,
            "duplicate_query_parameter",
            boost::beast::http::status::bad_request,
            "duplicate query parameter",
            "같은 쿼리 항목이 중복되었습니다.",
            "{field} 항목이 중복되었습니다.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::invalid_query_parameter,
            "invalid_query_parameter",
            boost::beast::http::status::bad_request,
            "invalid query parameter",
            "쿼리 항목이 올바르지 않습니다.",
            "{field} 항목이 올바르지 않습니다.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::unsupported_query_parameter,
            "unsupported_query_parameter",
            boost::beast::http::status::bad_request,
            "unsupported query parameter",
            "지원되지 않는 쿼리 항목입니다.",
            "{field} 항목은 지원되지 않습니다.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::missing_field,
            "missing_field",
            boost::beast::http::status::bad_request,
            "missing field",
            "필수 입력 항목을 입력해 주세요.",
            "{field} 항목을 입력해 주세요.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::invalid_field,
            "invalid_field",
            boost::beast::http::status::bad_request,
            "invalid field",
            "입력 항목이 올바르지 않습니다.",
            "{field} 항목이 올바르지 않습니다.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::invalid_length,
            "invalid_length",
            boost::beast::http::status::bad_request,
            "invalid length",
            "입력 길이가 올바르지 않습니다.",
            "{field} 항목 길이를 확인해 주세요.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::invalid_argument,
            "invalid_argument",
            boost::beast::http::status::bad_request,
            "invalid argument",
            "입력값을 다시 확인해 주세요.",
            "{field} 항목을 다시 확인해 주세요.",
            false,
            true,
            "request"
        },
        http_error_spec{
            http_error_code::missing_or_invalid_bearer_token,
            "missing_or_invalid_bearer_token",
            boost::beast::http::status::unauthorized,
            "missing or invalid bearer token",
            "로그인이 필요합니다.",
            "",
            true,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::invalid_or_expired_token,
            "invalid_or_expired_token",
            boost::beast::http::status::unauthorized,
            "invalid, expired, or revoked token",
            "로그인이 만료되었습니다. 다시 로그인해 주세요.",
            "",
            true,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::admin_bearer_token_required,
            "admin_bearer_token_required",
            boost::beast::http::status::unauthorized,
            "admin bearer token required",
            "관리자 권한이 필요합니다.",
            "",
            true,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::superadmin_bearer_token_required,
            "superadmin_bearer_token_required",
            boost::beast::http::status::unauthorized,
            "superadmin bearer token required",
            "슈퍼어드민 권한이 필요합니다.",
            "",
            true,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::invalid_credentials,
            "invalid_credentials",
            boost::beast::http::status::unauthorized,
            "invalid credentials",
            "ID 또는 비밀번호가 올바르지 않습니다.",
            "",
            false,
            false,
            "auth"
        },
        http_error_spec{
            http_error_code::submission_banned,
            "submission_banned",
            boost::beast::http::status::forbidden,
            "submission is currently banned",
            "현재 제출이 제한되어 있습니다.",
            "",
            false,
            false,
            "policy"
        },
        http_error_spec{
            http_error_code::invalid_testcase_zip,
            "invalid_testcase_zip",
            boost::beast::http::status::bad_request,
            "invalid testcase zip",
            "ZIP 파일 구성이 올바르지 않습니다.",
            "",
            false,
            false,
            "request"
        }
    }};
}

std::span<const http_error_spec> all_http_error_specs(){
    return http_error_specs;
}

const http_error_spec* find_http_error_spec(http_error_code code){
    for(const auto& spec : http_error_specs){
        if(spec.code == code){
            return &spec;
        }
    }

    return nullptr;
}

