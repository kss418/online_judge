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

export async function requestJson(path, options = {}){
  const { bearerToken, ...fetchOptions } = options
  const headers = new Headers(fetchOptions.headers || {})
  headers.set('Accept', 'application/json')

  if (bearerToken) {
    headers.set('Authorization', `Bearer ${bearerToken}`)
  }

  const hasJsonBody =
    fetchOptions.body &&
    !(fetchOptions.body instanceof FormData) &&
    !(fetchOptions.body instanceof URLSearchParams) &&
    typeof fetchOptions.body !== 'string'

  if (hasJsonBody && !headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json')
  }

  const response = await fetch(`${apiBaseUrl}${path}`, {
    ...fetchOptions,
    body: hasJsonBody ? JSON.stringify(fetchOptions.body) : fetchOptions.body,
    headers
  })

  const payload = await parseResponse(response)

  if (!response.ok) {
    const errorMessage =
      typeof payload === 'string'
        ? payload
        : payload?.error?.message || payload?.message || `Request failed with status ${response.status}`

    const error = new Error(errorMessage)
    error.status = response.status
    error.code = payload?.error?.code || null
    error.payload = payload
    throw error
  }

  return payload
}

export function getSystemHealth(){
  return requestJson('/system/health')
}

export function getSupportedLanguages(){
  return requestJson('/system/supported-languages')
}
