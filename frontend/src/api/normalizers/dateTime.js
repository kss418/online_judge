import { normalizeTrimmedString } from '@/api/normalizers/common'

export function normalizeApiDateTime(value){
  const rawValue = normalizeTrimmedString(value)
  if (!rawValue) {
    return {
      value: null,
      timestamp: null,
      label: ''
    }
  }

  const matchedTimestamp = rawValue.match(
    /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
  )

  if (matchedTimestamp) {
    const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
      matchedTimestamp
    const normalizedFraction = fractionPart
      ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
      : ''
    const parsedTimestamp = Date.parse(
      `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
    )

    return {
      value: rawValue,
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(rawValue.replace(' ', 'T'))
  return {
    value: rawValue,
    timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    label: rawValue
  }
}

export function normalizeApiDateTimeField(fieldName, value){
  const normalizedDateTime = normalizeApiDateTime(value)

  return {
    [fieldName]: normalizedDateTime.value,
    [`${fieldName}_timestamp`]: normalizedDateTime.timestamp,
    [`${fieldName}_label`]: normalizedDateTime.label
  }
}
