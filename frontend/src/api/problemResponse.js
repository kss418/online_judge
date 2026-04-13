import {
  normalizeArray,
  normalizeNumber,
  normalizePositiveInteger,
  normalizePositiveIntegerOrNull,
  normalizeString
} from '@/api/normalizers/common'
import {
  normalizeProblemStateRecord
} from '@/api/normalizers/problemState'

export function normalizeProblemLimits(payload){
  return {
    time_limit_ms: normalizePositiveInteger(payload?.time_limit_ms),
    memory_limit_mb: normalizePositiveInteger(payload?.memory_limit_mb)
  }
}

function normalizeProblemStatistics(payload){
  return {
    accepted_count: normalizePositiveInteger(payload?.accepted_count),
    submission_count: normalizePositiveInteger(payload?.submission_count),
    acceptance_rate: normalizeNumber(payload?.acceptance_rate, 0)
  }
}

function normalizeProblemSample(sample){
  return {
    sample_order: normalizePositiveInteger(sample?.sample_order),
    sample_input: normalizeString(sample?.sample_input),
    sample_output: normalizeString(sample?.sample_output)
  }
}

function normalizeProblemSamples(samples){
  return normalizeArray(samples)
    .map(normalizeProblemSample)
    .filter((sample) => sample.sample_order > 0)
    .sort((leftSample, rightSample) => leftSample.sample_order - rightSample.sample_order)
}

function normalizeProblemListItem(problem){
  return normalizeProblemStateRecord({
    problem_id: normalizePositiveInteger(problem?.problem_id),
    title: normalizeString(problem?.title),
    version: normalizePositiveInteger(problem?.version),
    time_limit_ms: normalizePositiveInteger(problem?.time_limit_ms ?? problem?.limits?.time_limit_ms),
    memory_limit_mb: normalizePositiveInteger(problem?.memory_limit_mb ?? problem?.limits?.memory_limit_mb),
    accepted_count: normalizePositiveInteger(problem?.accepted_count),
    submission_count: normalizePositiveInteger(problem?.submission_count),
    acceptance_rate: normalizeNumber(problem?.acceptance_rate, 0),
    user_problem_state: problem?.user_problem_state
  })
}

export function normalizeProblemDetailResponse(problem){
  const normalizedProblem = normalizeProblemStateRecord({
    problem_id: normalizePositiveInteger(problem?.problem_id),
    title: normalizeString(problem?.title),
    version: normalizePositiveInteger(problem?.version),
    user_problem_state: problem?.user_problem_state
  })

  return {
    ...normalizedProblem,
    limits: normalizeProblemLimits(problem?.limits),
    statistics: normalizeProblemStatistics(problem?.statistics),
    statement: {
      description: normalizeString(problem?.statement?.description),
      input_format: normalizeString(problem?.statement?.input_format),
      output_format: normalizeString(problem?.statement?.output_format),
      note: normalizeString(problem?.statement?.note)
    },
    samples: normalizeProblemSamples(problem?.samples)
  }
}

export function normalizeProblemListResponse(response){
  return {
    problem_count: normalizePositiveInteger(response?.problem_count),
    total_problem_count: normalizePositiveInteger(
      response?.total_problem_count ?? response?.problem_count
    ),
    problems: normalizeArray(response?.problems).map(normalizeProblemListItem)
  }
}

export function normalizeProblemVersionResponse(response){
  return {
    version: normalizePositiveIntegerOrNull(response?.version)
  }
}

export function normalizeProblemCreateResponse(response){
  return {
    problem_id: normalizePositiveInteger(response?.problem_id),
    version: normalizePositiveIntegerOrNull(response?.version),
    title: normalizeString(response?.title)
  }
}

export function normalizeProblemSampleResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    sample_order: normalizePositiveInteger(response?.sample_order),
    sample_input: normalizeString(response?.sample_input),
    sample_output: normalizeString(response?.sample_output)
  }
}

export function normalizeProblemSamplesResponse(response){
  return {
    samples: normalizeProblemSamples(response?.samples)
  }
}

function normalizeProblemTestcaseSummary(testcase){
  return {
    testcase_id: normalizePositiveInteger(testcase?.testcase_id),
    testcase_order: normalizePositiveInteger(testcase?.testcase_order),
    input_char_count: normalizePositiveInteger(testcase?.input_char_count),
    input_line_count: normalizePositiveInteger(testcase?.input_line_count),
    output_char_count: normalizePositiveInteger(testcase?.output_char_count),
    output_line_count: normalizePositiveInteger(testcase?.output_line_count)
  }
}

export function normalizeProblemTestcasesResponse(response){
  return {
    testcase_count: normalizePositiveInteger(
      response?.testcase_count ?? normalizeArray(response?.testcases).length
    ),
    testcases: normalizeArray(response?.testcases).map(normalizeProblemTestcaseSummary)
  }
}

export function normalizeProblemTestcaseResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    testcase_id: normalizePositiveInteger(response?.testcase_id),
    testcase_order: normalizePositiveInteger(response?.testcase_order),
    testcase_input: normalizeString(response?.testcase_input),
    testcase_output: normalizeString(response?.testcase_output)
  }
}

export function normalizeProblemTestcaseZipResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    testcase_count: normalizePositiveInteger(response?.testcase_count)
  }
}
