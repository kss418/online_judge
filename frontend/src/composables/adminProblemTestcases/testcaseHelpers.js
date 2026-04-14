import { formatCount } from '@/utils/numberFormat'

export function formatProblemLimit(value, unit){
  const numericValue = Number(value ?? 0)
  if (!numericValue) {
    return `미설정 ${unit}`
  }

  return `${formatCount(numericValue)} ${unit}`
}
