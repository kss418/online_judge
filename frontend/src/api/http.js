import { formatApiError } from '@/utils/apiError'

const rawApiBaseUrl = import.meta.env.VITE_API_BASE_URL || '/api'

export const apiBaseUrl = rawApiBaseUrl.endsWith('/')
  ? rawApiBaseUrl.slice(0, -1)
  : rawApiBaseUrl

async function parseResponse(response){
  const contentType = response.headers.get('content-type') || ''

  if (contentType.includes('application/json')) {
    return response.json()
  }

  return response.text()
}

function createRequestError(details){
  const error = new Error(formatApiError(details))
  error.status = Number(details.status ?? 0)
  error.code = details.code || null
  error.field = details.field || null
  error.rawMessage = details.message || ''
  error.payload = details.payload ?? null
  return error
}

export async function requestJson(path, options = {}){
  const { bearerToken, ...fetchOptions } = options
  const headers = new Headers(fetchOptions.headers || {})
  headers.set('Accept', 'application/json')

  if (bearerToken) {
    headers.set('Authorization', `Bearer ${bearerToken}`)
  }

  const isBlobBody =
    typeof Blob !== 'undefined' &&
    fetchOptions.body instanceof Blob
  const isArrayBufferBody =
    typeof ArrayBuffer !== 'undefined' &&
    (fetchOptions.body instanceof ArrayBuffer || ArrayBuffer.isView(fetchOptions.body))
  const hasJsonBody =
    fetchOptions.body &&
    !isBlobBody &&
    !isArrayBufferBody &&
    !(fetchOptions.body instanceof FormData) &&
    !(fetchOptions.body instanceof URLSearchParams) &&
    typeof fetchOptions.body !== 'string'

  if (hasJsonBody && !headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json')
  }

  let response

  try {
    response = await fetch(`${apiBaseUrl}${path}`, {
      ...fetchOptions,
      body: hasJsonBody ? JSON.stringify(fetchOptions.body) : fetchOptions.body,
      headers
    })
  } catch (error) {
    throw createRequestError({
      status: 0,
      code: 'network_error',
      field: '',
      message: error instanceof Error ? error.message : '',
      payload: null
    })
  }

  let payload

  try {
    payload = await parseResponse(response)
  } catch (error) {
    throw createRequestError({
      status: response.status,
      code: 'response_parse_error',
      field: '',
      message: error instanceof Error ? error.message : '',
      payload: null
    })
  }

  if (!response.ok) {
    const errorMessage =
      typeof payload === 'string'
        ? payload
        : payload?.error?.message || payload?.message || `Request failed with status ${response.status}`
    const errorCode = typeof payload?.error?.code === 'string' ? payload.error.code : ''
    const errorField = typeof payload?.error?.field === 'string' ? payload.error.field : ''

    throw createRequestError({
      status: response.status,
      code: errorCode,
      field: errorField,
      message: errorMessage,
      payload
    })
  }

  return payload
}

export function getSystemHealth(){
  return requestJson('/system/health')
}

export function getSupportedLanguages(){
  return requestJson('/system/supported-languages')
}
