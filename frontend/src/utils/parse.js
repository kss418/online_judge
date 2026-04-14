export function parsePositiveInteger(value){
  const normalizedValue = typeof value === 'string'
    ? value.trim()
    : String(value ?? '').trim()

  if (!normalizedValue) {
    return null
  }

  const parsedValue = Number.parseInt(normalizedValue, 10)

  return Number.isInteger(parsedValue) && parsedValue > 0
    ? parsedValue
    : null
}
