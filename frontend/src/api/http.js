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
  const headers = new Headers(options.headers || {})
  headers.set('Accept', 'application/json')

  const hasJsonBody =
    options.body &&
    !(options.body instanceof FormData) &&
    !(options.body instanceof URLSearchParams) &&
    typeof options.body !== 'string'

  if (hasJsonBody && !headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json')
  }

  const response = await fetch(`${apiBaseUrl}${path}`, {
    ...options,
    body: hasJsonBody ? JSON.stringify(options.body) : options.body,
    headers
  })

  const payload = await parseResponse(response)

  if (!response.ok) {
    const errorMessage =
      typeof payload === 'string'
        ? payload
        : payload?.message || `Request failed with status ${response.status}`

    throw new Error(errorMessage)
  }

  return payload
}

export function getSystemHealth(){
  return requestJson('/system/health')
}

export function getSupportedLanguages(){
  return requestJson('/system/supported-languages')
}
