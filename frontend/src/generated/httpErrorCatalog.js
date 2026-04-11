export const httpErrorCatalog = [
  {
    "code": "bad_request",
    "http_status": 400,
    "default_public_message_en": "bad request",
    "frontend_message_ko": "요청 형식이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "protocol"
  },
  {
    "code": "payload_too_large",
    "http_status": 413,
    "default_public_message_en": "request body too large",
    "frontend_message_ko": "요청 본문이 너무 큽니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "protocol"
  },
  {
    "code": "method_not_allowed",
    "http_status": 405,
    "default_public_message_en": "method not allowed",
    "frontend_message_ko": "지원되지 않는 요청 방식입니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "protocol"
  },
  {
    "code": "validation_error",
    "http_status": 400,
    "default_public_message_en": "validation error",
    "frontend_message_ko": "입력값을 다시 확인해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "validation"
  },
  {
    "code": "unauthorized",
    "http_status": 401,
    "default_public_message_en": "unauthorized",
    "frontend_message_ko": "로그인이 필요합니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "forbidden",
    "http_status": 403,
    "default_public_message_en": "forbidden",
    "frontend_message_ko": "권한이 없습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "not_found",
    "http_status": 404,
    "default_public_message_en": "not found",
    "frontend_message_ko": "요청한 대상을 찾지 못했습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "resource"
  },
  {
    "code": "conflict",
    "http_status": 409,
    "default_public_message_en": "conflict",
    "frontend_message_ko": "이미 존재하거나 현재 상태와 충돌합니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "state"
  },
  {
    "code": "service_unavailable",
    "http_status": 503,
    "default_public_message_en": "service unavailable",
    "frontend_message_ko": "서비스가 일시적으로 불안정합니다. 잠시 후 다시 시도해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "availability"
  },
  {
    "code": "internal_server_error",
    "http_status": 500,
    "default_public_message_en": "internal server error",
    "frontend_message_ko": "서버에서 요청을 처리하지 못했습니다. 잠시 후 다시 시도해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "server"
  },
  {
    "code": "invalid_json",
    "http_status": 400,
    "default_public_message_en": "invalid json",
    "frontend_message_ko": "요청 본문 JSON 형식이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "request"
  },
  {
    "code": "invalid_query_string",
    "http_status": 400,
    "default_public_message_en": "invalid query string",
    "frontend_message_ko": "쿼리 문자열 형식이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "request"
  },
  {
    "code": "duplicate_query_parameter",
    "http_status": 400,
    "default_public_message_en": "duplicate query parameter",
    "frontend_message_ko": "같은 쿼리 항목이 중복되었습니다.",
    "frontend_field_message_ko": "{field} 항목이 중복되었습니다.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "invalid_query_parameter",
    "http_status": 400,
    "default_public_message_en": "invalid query parameter",
    "frontend_message_ko": "쿼리 항목이 올바르지 않습니다.",
    "frontend_field_message_ko": "{field} 항목이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "unsupported_query_parameter",
    "http_status": 400,
    "default_public_message_en": "unsupported query parameter",
    "frontend_message_ko": "지원되지 않는 쿼리 항목입니다.",
    "frontend_field_message_ko": "{field} 항목은 지원되지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "missing_field",
    "http_status": 400,
    "default_public_message_en": "missing field",
    "frontend_message_ko": "필수 입력 항목을 입력해 주세요.",
    "frontend_field_message_ko": "{field} 항목을 입력해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "invalid_field",
    "http_status": 400,
    "default_public_message_en": "invalid field",
    "frontend_message_ko": "입력 항목이 올바르지 않습니다.",
    "frontend_field_message_ko": "{field} 항목이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "invalid_length",
    "http_status": 400,
    "default_public_message_en": "invalid length",
    "frontend_message_ko": "입력 길이가 올바르지 않습니다.",
    "frontend_field_message_ko": "{field} 항목 길이를 확인해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "invalid_argument",
    "http_status": 400,
    "default_public_message_en": "invalid argument",
    "frontend_message_ko": "입력값을 다시 확인해 주세요.",
    "frontend_field_message_ko": "{field} 항목을 다시 확인해 주세요.",
    "requires_bearer_auth": false,
    "field_supported": true,
    "category": "request"
  },
  {
    "code": "missing_or_invalid_bearer_token",
    "http_status": 401,
    "default_public_message_en": "missing or invalid bearer token",
    "frontend_message_ko": "로그인이 필요합니다.",
    "requires_bearer_auth": true,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "invalid_or_expired_token",
    "http_status": 401,
    "default_public_message_en": "invalid, expired, or revoked token",
    "frontend_message_ko": "로그인이 만료되었습니다. 다시 로그인해 주세요.",
    "requires_bearer_auth": true,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "admin_bearer_token_required",
    "http_status": 401,
    "default_public_message_en": "admin bearer token required",
    "frontend_message_ko": "관리자 권한이 필요합니다.",
    "requires_bearer_auth": true,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "superadmin_bearer_token_required",
    "http_status": 401,
    "default_public_message_en": "superadmin bearer token required",
    "frontend_message_ko": "슈퍼어드민 권한이 필요합니다.",
    "requires_bearer_auth": true,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "invalid_credentials",
    "http_status": 401,
    "default_public_message_en": "invalid credentials",
    "frontend_message_ko": "ID 또는 비밀번호가 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "auth"
  },
  {
    "code": "submission_banned",
    "http_status": 403,
    "default_public_message_en": "submission is currently banned",
    "frontend_message_ko": "현재 제출이 제한되어 있습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "policy"
  },
  {
    "code": "invalid_testcase_zip",
    "http_status": 400,
    "default_public_message_en": "invalid testcase zip",
    "frontend_message_ko": "ZIP 파일 구성이 올바르지 않습니다.",
    "requires_bearer_auth": false,
    "field_supported": false,
    "category": "request"
  }
]

const httpErrorMetaByCode = new Map(
  httpErrorCatalog.map((meta) => [meta.code, meta])
)

function normalizeCode(code){
  return typeof code === 'string' ? code.trim() : ''
}

export function getHttpErrorMeta(code){
  const normalizedCode = normalizeCode(code)
  return httpErrorMetaByCode.get(normalizedCode) || null
}

export function getFrontendHttpErrorMessage(code, options = {}){
  const meta = getHttpErrorMeta(code)
  if (!meta) {
    return ''
  }

  const fieldLabel = typeof options.fieldLabel === 'string'
    ? options.fieldLabel.trim()
    : ''

  if (meta.field_supported && fieldLabel && typeof meta.frontend_field_message_ko === 'string') {
    return meta.frontend_field_message_ko.replace('{field}', fieldLabel)
  }

  return typeof meta.frontend_message_ko === 'string'
    ? meta.frontend_message_ko
    : ''
}

