export function normalizeString(value, fallback = ''){
  return typeof value === 'string' ? value : fallback
}

export function normalizeTrimmedString(value, fallback = ''){
  return typeof value === 'string' ? value.trim() : fallback
}

export function normalizeNumber(value, fallback = 0){
  const numericValue = Number(value)
  return Number.isFinite(numericValue) ? numericValue : fallback
}

export function normalizeInteger(value, fallback = 0){
  const numericValue = Number(value)
  return Number.isInteger(numericValue) ? numericValue : fallback
}

export function normalizePositiveInteger(value, fallback = 0){
  const numericValue = normalizeInteger(value, fallback)
  return numericValue > 0 ? numericValue : fallback
}

export function normalizePositiveIntegerOrNull(value){
  const numericValue = Number(value)
  return Number.isInteger(numericValue) && numericValue > 0
    ? numericValue
    : null
}

export function normalizeNullableNumber(value){
  if (value === null || typeof value === 'undefined' || value === '') {
    return null
  }

  const numericValue = Number(value)
  return Number.isFinite(numericValue) ? numericValue : null
}

export function normalizeBoolean(value){
  return Boolean(value)
}

export function normalizeArray(value){
  return Array.isArray(value) ? value : []
}
