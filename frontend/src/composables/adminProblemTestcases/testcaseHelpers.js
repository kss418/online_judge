export function formatCount(countFormatter, value){
  return countFormatter.format(Number(value) || 0)
}

export function formatProblemLimit(countFormatter, value, unit){
  const numericValue = Number(value ?? 0)
  if (!numericValue) {
    return `미설정 ${unit}`
  }

  return `${formatCount(countFormatter, numericValue)} ${unit}`
}

export function parsePositiveInteger(value){
  const normalized = typeof value === 'string' ? value.trim() : String(value ?? '').trim()
  if (!normalized) {
    return null
  }

  const parsedValue = Number.parseInt(normalized, 10)
  if (!Number.isInteger(parsedValue) || parsedValue <= 0) {
    return null
  }

  return parsedValue
}
