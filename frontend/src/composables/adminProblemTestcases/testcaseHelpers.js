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

export function normalizeProblemItem(problem){
  return {
    ...problem,
    problem_id: Number(problem.problem_id ?? 0),
    version: Number(problem.version ?? 1),
    title: problem.title || '',
    time_limit_ms: Number(problem.time_limit_ms ?? problem.limits?.time_limit_ms ?? 0),
    memory_limit_mb: Number(problem.memory_limit_mb ?? problem.limits?.memory_limit_mb ?? 0)
  }
}

export function normalizeProblemDetail(detail){
  return {
    ...detail,
    problem_id: Number(detail.problem_id ?? 0),
    version: Number(detail.version ?? 1),
    title: detail.title || ''
  }
}

export function normalizeTestcaseList(response){
  const rawTestcases = Array.isArray(response?.testcases) ? response.testcases : []

  return rawTestcases.map((testcase) => ({
    testcase_id: Number(testcase.testcase_id ?? 0),
    testcase_order: Number(testcase.testcase_order ?? 0),
    input_char_count: Number(testcase.input_char_count ?? 0),
    input_line_count: Number(testcase.input_line_count ?? 0),
    output_char_count: Number(testcase.output_char_count ?? 0),
    output_line_count: Number(testcase.output_line_count ?? 0)
  }))
}

export function normalizeTestcaseDetail(response){
  return {
    testcase_id: Number(response?.testcase_id ?? 0),
    testcase_order: Number(response?.testcase_order ?? 0),
    testcase_input: typeof response?.testcase_input === 'string' ? response.testcase_input : '',
    testcase_output: typeof response?.testcase_output === 'string' ? response.testcase_output : ''
  }
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
