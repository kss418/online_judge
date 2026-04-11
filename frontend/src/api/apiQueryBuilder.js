function normalizeQueryScalar(value){
  if (value === null || typeof value === 'undefined') {
    return undefined
  }

  if (typeof value === 'string') {
    const trimmedValue = value.trim()
    return trimmedValue ? trimmedValue : undefined
  }

  if (typeof value === 'number') {
    return Number.isFinite(value) ? String(value) : undefined
  }

  if (typeof value === 'boolean') {
    return value ? 'true' : 'false'
  }

  const normalizedValue = String(value).trim()
  return normalizedValue ? normalizedValue : undefined
}

function normalizeQueryEntryValue(value){
  if (Array.isArray(value)) {
    return value
      .map(normalizeQueryScalar)
      .filter((item) => typeof item !== 'undefined')
  }

  return normalizeQueryScalar(value)
}

export function buildApiQuery(query){
  const normalizedQuery = {}

  Object.entries(query ?? {}).forEach(([key, value]) => {
    const normalizedValue = normalizeQueryEntryValue(value)

    if (Array.isArray(normalizedValue)) {
      if (normalizedValue.length > 0) {
        normalizedQuery[key] = normalizedValue
      }
      return
    }

    if (typeof normalizedValue !== 'undefined') {
      normalizedQuery[key] = normalizedValue
    }
  })

  return normalizedQuery
}

export function buildApiQueryString(query){
  const normalizedQuery = buildApiQuery(query)
  const searchParams = new URLSearchParams()

  Object.entries(normalizedQuery).forEach(([key, value]) => {
    if (Array.isArray(value)) {
      value.forEach((item) => {
        searchParams.append(key, item)
      })
      return
    }

    searchParams.set(key, value)
  })

  return searchParams.toString()
}

export function appendApiQuery(path, query){
  const queryString = buildApiQueryString(query)
  return queryString ? `${path}?${queryString}` : path
}
