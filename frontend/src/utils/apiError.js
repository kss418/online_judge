import { getFrontendHttpErrorMessage } from '@/generated/httpErrorCatalog'

const defaultFieldLabels = {
  user_login_id: 'ID',
  raw_password: '비밀번호',
  title: '제목',
  description: '문제 설명',
  input_format: '입력 설명',
  output_format: '출력 설명',
  note: '비고',
  sample_input: '예제 입력',
  sample_output: '예제 출력',
  testcase_input: '테스트케이스 입력',
  testcase_output: '테스트케이스 출력',
  language: '언어',
  source_code: '소스 코드',
  time_limit_ms: '시간 제한',
  memory_limit_mb: '메모리 제한',
  duration_minutes: '제출 제한 시간',
  source_testcase_order: '기존 테스트케이스 순번',
  target_testcase_order: '대상 테스트케이스 순번',
  q: '검색어',
  state: '상태',
  problem_id: '문제 번호',
  submission_id: '제출 번호',
  submission_ids: '제출 번호 목록',
  testcase_order: '테스트케이스 순번',
  user_id: '사용자 번호'
}

const defaultStatusMessages = {
  400: '요청 형식이 올바르지 않습니다.',
  401: '로그인이 필요합니다.',
  403: '권한이 없습니다.',
  404: '요청한 대상을 찾지 못했습니다.',
  409: '이미 존재하거나 현재 상태와 충돌합니다.',
  413: '요청 본문이 너무 큽니다.',
  500: '서버에서 요청을 처리하지 못했습니다. 잠시 후 다시 시도해 주세요.',
  502: '서버 연결이 원활하지 않습니다. 잠시 후 다시 시도해 주세요.',
  503: '서비스가 일시적으로 불안정합니다. 잠시 후 다시 시도해 주세요.',
  504: '서버 응답이 지연되고 있습니다. 잠시 후 다시 시도해 주세요.'
}

const frontendOnlyCodeMessages = {
  network_error: '서버에 연결하지 못했습니다. 네트워크 상태를 확인해 주세요.',
  response_parse_error: '서버 응답을 해석하지 못했습니다.'
}

function normalizeString(value){
  return typeof value === 'string' ? value.trim() : ''
}

function normalizeStatus(value){
  const numericValue = Number(value)
  return Number.isInteger(numericValue) && numericValue >= 0 ? numericValue : 0
}

function looksLikeNetworkFailure(message){
  const normalizedMessage = normalizeString(message).toLowerCase()
  if (!normalizedMessage) {
    return false
  }

  return (
    normalizedMessage.includes('failed to fetch') ||
    normalizedMessage.includes('networkerror') ||
    normalizedMessage.includes('load failed')
  )
}

function resolveFieldLabel(field, fieldLabels){
  const normalizedField = normalizeString(field)
  if (!normalizedField) {
    return ''
  }

  const mergedFieldLabels = {
    ...defaultFieldLabels,
    ...(fieldLabels || {})
  }

  return normalizeString(mergedFieldLabels[normalizedField]) || normalizedField
}

function resolveCodeMessage(messageSource, context){
  if (typeof messageSource === 'function') {
    return normalizeString(messageSource(context))
  }

  return normalizeString(messageSource)
}

export function getApiErrorDetails(error){
  const payload =
    error && typeof error === 'object' && 'payload' in error
      ? error.payload
      : null
  const payloadError =
    payload && typeof payload === 'object' && payload.error && typeof payload.error === 'object'
      ? payload.error
      : null

  const code = normalizeString(
    error && typeof error === 'object' && 'code' in error
      ? error.code
      : payloadError?.code
  )
  const field = normalizeString(
    error && typeof error === 'object' && 'field' in error
      ? error.field
      : payloadError?.field
  )
  const message = normalizeString(
    error && typeof error === 'object' && 'rawMessage' in error
      ? error.rawMessage
      : payloadError?.message || payload?.message || error?.message
  )

  return {
    kind:
      error && typeof error === 'object' && 'kind' in error && typeof error.kind === 'string'
        ? error.kind
        : '',
    status: normalizeStatus(error?.status),
    code,
    field,
    message,
    payload
  }
}

export function getApiErrorField(error){
  return getApiErrorDetails(error).field || null
}

export function formatApiError(error, options = {}){
  const details = getApiErrorDetails(error)
  const fieldLabel = resolveFieldLabel(details.field, options.fieldLabels)
  const context = {
    ...details,
    fieldLabel,
    fallback: normalizeString(options.fallback)
  }

  if (details.code) {
    const overrideCodeMessage = resolveCodeMessage(options.codeMessages?.[details.code], context)
    if (overrideCodeMessage) {
      return overrideCodeMessage
    }

    const frontendOnlyMessage = resolveCodeMessage(frontendOnlyCodeMessages[details.code], context)
    if (frontendOnlyMessage) {
      return frontendOnlyMessage
    }

    const catalogMessage = normalizeString(
      getFrontendHttpErrorMessage(details.code, {
        fieldLabel
      })
    )
    if (catalogMessage) {
      return catalogMessage
    }
  }

  if (looksLikeNetworkFailure(details.message)) {
    return frontendOnlyCodeMessages.network_error
  }

  if (details.message && details.code === '') {
    return details.message
  }

  if (details.status > 0) {
    const statusMessage = defaultStatusMessages[details.status]
    if (statusMessage) {
      return statusMessage
    }

    if (details.status >= 500) {
      return defaultStatusMessages[500]
    }
  }

  if (details.message) {
    return details.message
  }

  return normalizeString(options.fallback) || '요청을 처리하지 못했습니다.'
}
