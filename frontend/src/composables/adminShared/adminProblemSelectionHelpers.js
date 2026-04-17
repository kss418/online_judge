import { unref } from 'vue'

import { formatCount } from '@/utils/numberFormat'

export function unwrapRecordValues(record){
  const nextRecord = {}

  Object.entries(record ?? {}).forEach(([key, value]) => {
    nextRecord[key] = unref(value)
  })

  return nextRecord
}

export function normalizeAdminProblemId(value){
  const normalizedProblemId = Number(value)

  return Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
    ? normalizedProblemId
    : 0
}

export function resolveAdminProblemId(valueOrRef){
  return normalizeAdminProblemId(unref(valueOrRef))
}

export function sanitizeNumericInputValue(value){
  return String(value ?? '').replace(/\D+/g, '')
}

export function formatAdminProblemLimit(value, unit){
  const numericValue = Number(value)

  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return `미설정 ${unit}`
  }

  return `${formatCount(numericValue)} ${unit}`
}
