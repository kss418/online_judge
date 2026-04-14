import { formatCount } from '@/utils/numberFormat'

export function formatProblemLimit(value, unit){
  const numericValue = Number(value)

  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return unit === 'ms' ? '시간 확인 중' : '메모리 확인 중'
  }

  return `${formatCount(numericValue)} ${unit}`
}

export function makeSampleDraft(sample){
  return {
    sample_order: sample.sample_order,
    sample_input: sample.sample_input,
    sample_output: sample.sample_output
  }
}
