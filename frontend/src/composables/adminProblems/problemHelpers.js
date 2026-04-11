export function formatCount(countFormatter, value){
  return countFormatter.format(Number(value ?? 0))
}

export function formatProblemLimit(countFormatter, value, unit){
  const numericValue = Number(value)

  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return unit === 'ms' ? '시간 확인 중' : '메모리 확인 중'
  }

  return `${formatCount(countFormatter, numericValue)} ${unit}`
}

export function parsePositiveInteger(value){
  const numericValue = Number.parseInt(String(value), 10)
  return Number.isInteger(numericValue) && numericValue > 0 ? numericValue : null
}

export function makeSampleDraft(sample){
  return {
    sample_order: sample.sample_order,
    sample_input: sample.sample_input,
    sample_output: sample.sample_output
  }
}
