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

export function normalizeProblemSamples(samples){
  if (!Array.isArray(samples)) {
    return []
  }

  return samples
    .map((sample) => ({
      sample_order: Number(sample.sample_order ?? 0),
      sample_input: sample.sample_input ?? '',
      sample_output: sample.sample_output ?? ''
    }))
    .filter((sample) => sample.sample_order > 0)
    .sort((leftSample, rightSample) => leftSample.sample_order - rightSample.sample_order)
}

export function normalizeProblemDetail(response){
  return {
    problem_id: Number(response.problem_id ?? 0),
    title: response.title ?? '',
    version: Number(response.version ?? 0),
    limits: {
      time_limit_ms: Number(response.limits?.time_limit_ms ?? 0),
      memory_limit_mb: Number(response.limits?.memory_limit_mb ?? 0)
    },
    statement: {
      description: response.statement?.description ?? '',
      input_format: response.statement?.input_format ?? '',
      output_format: response.statement?.output_format ?? '',
      note: response.statement?.note ?? ''
    },
    samples: normalizeProblemSamples(response.samples)
  }
}

export function makeSampleDraft(sample){
  return {
    sample_order: sample.sample_order,
    sample_input: sample.sample_input,
    sample_output: sample.sample_output
  }
}
