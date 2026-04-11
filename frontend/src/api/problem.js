import { requestJson } from '@/api/http'
import { appendApiQuery } from '@/api/apiQueryBuilder'
import {
  normalizeArray,
  normalizeBoolean,
  normalizeNumber,
  normalizePositiveInteger,
  normalizePositiveIntegerOrNull,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'
import {
  normalizeProblemStateRecord,
  normalizeProblemStateRecords
} from '@/api/normalizers/problemState'

function normalizeProblemLimits(payload){
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
  const normalizedProblem = normalizeProblemStateRecord({
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

  return normalizedProblem
}

function normalizeProblemDetailResponse(problem){
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

function normalizeProblemListResponse(response){
  return {
    problem_count: normalizePositiveInteger(response?.problem_count),
    total_problem_count: normalizePositiveInteger(
      response?.total_problem_count ?? response?.problem_count
    ),
    problems: normalizeArray(response?.problems).map(normalizeProblemListItem)
  }
}

function normalizeProblemVersionResponse(response){
  return {
    version: normalizePositiveIntegerOrNull(response?.version)
  }
}

function normalizeProblemCreateResponse(response){
  return {
    problem_id: normalizePositiveInteger(response?.problem_id),
    version: normalizePositiveIntegerOrNull(response?.version),
    title: normalizeString(response?.title)
  }
}

function normalizeProblemSampleResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    sample_order: normalizePositiveInteger(response?.sample_order),
    sample_input: normalizeString(response?.sample_input),
    sample_output: normalizeString(response?.sample_output)
  }
}

function normalizeProblemSamplesResponse(response){
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

function normalizeProblemTestcasesResponse(response){
  return {
    testcase_count: normalizePositiveInteger(
      response?.testcase_count ?? normalizeArray(response?.testcases).length
    ),
    testcases: normalizeArray(response?.testcases).map(normalizeProblemTestcaseSummary)
  }
}

function normalizeProblemTestcaseResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    testcase_id: normalizePositiveInteger(response?.testcase_id),
    testcase_order: normalizePositiveInteger(response?.testcase_order),
    testcase_input: normalizeString(response?.testcase_input),
    testcase_output: normalizeString(response?.testcase_output)
  }
}

function normalizeProblemTestcaseZipResponse(response){
  return {
    ...normalizeProblemVersionResponse(response),
    testcase_count: normalizePositiveInteger(response?.testcase_count)
  }
}

export async function getProblemList(options = {}){
  const {
    bearerToken = '',
    title = '',
    problemId = null,
    state = '',
    sort = '',
    direction = '',
    limit = null,
    offset = null
  } = options
  const path = appendApiQuery('/problem', {
    title: normalizeTrimmedString(title),
    problem_id: normalizePositiveIntegerOrNull(problemId),
    state: normalizeTrimmedString(state),
    sort: normalizeTrimmedString(sort),
    direction: normalizeTrimmedString(direction),
    limit: normalizePositiveIntegerOrNull(limit),
    offset: Number.isInteger(offset) && offset >= 0 ? offset : null
  })

  const response = await requestJson(path, {
    bearerToken
  })

  return normalizeProblemListResponse(response)
}

export async function getProblemDetail(problemId, options = {}){
  const { bearerToken = '' } = options

  const response = await requestJson(`/problem/${problemId}`, {
    bearerToken
  })

  return normalizeProblemDetailResponse(response)
}

export function createProblem(payload, token){
  return requestJson('/problem', {
    method: 'POST',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemCreateResponse)
}

export function updateProblemTitle(problemId, payload, token){
  return requestJson(`/problem/${problemId}/title`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function updateProblemLimits(problemId, payload, token){
  return requestJson(`/problem/${problemId}/limits`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function getProblemLimits(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/limits`, {
    bearerToken
  }).then(normalizeProblemLimits)
}

export function updateProblemStatement(problemId, payload, token){
  return requestJson(`/problem/${problemId}/statement`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function getProblemSamples(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/sample`, {
    bearerToken
  }).then(normalizeProblemSamplesResponse)
}

export function createProblemSample(problemId, token){
  return requestJson(`/problem/${problemId}/sample`, {
    method: 'POST',
    bearerToken: token
  }).then(normalizeProblemSampleResponse)
}

export function updateProblemSample(problemId, sampleOrder, payload, token){
  return requestJson(`/problem/${problemId}/sample/${sampleOrder}`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemSampleResponse)
}

export function deleteProblemSample(problemId, token){
  return requestJson(`/problem/${problemId}/sample`, {
    method: 'DELETE',
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function uploadProblemTestcaseZip(problemId, file, token){
  return requestJson(`/problem/${problemId}/testcase/zip`, {
    method: 'POST',
    body: file,
    bearerToken: token,
    headers: {
      'Content-Type': 'application/zip'
    }
  }).then(normalizeProblemTestcaseZipResponse)
}

export function getProblemTestcases(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/testcase`, {
    bearerToken
  }).then(normalizeProblemTestcasesResponse)
}

export function getProblemTestcase(problemId, testcaseOrder, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/testcase/${testcaseOrder}`, {
    bearerToken
  }).then(normalizeProblemTestcaseResponse)
}

export function createProblemTestcase(problemId, payload, token){
  return requestJson(`/problem/${problemId}/testcase`, {
    method: 'POST',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemTestcaseResponse)
}

export function deleteProblemTestcase(problemId, testcaseOrder, token){
  return requestJson(`/problem/${problemId}/testcase/${testcaseOrder}`, {
    method: 'DELETE',
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function moveProblemTestcase(problemId, payload, token){
  return requestJson(`/problem/${problemId}/testcase/move`, {
    method: 'POST',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
}

export function updateProblemTestcase(problemId, testcaseOrder, payload, token){
  return requestJson(`/problem/${problemId}/testcase/${testcaseOrder}`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemTestcaseResponse)
}

export function deleteProblem(problemId, token){
  return requestJson(`/problem/${problemId}`, {
    method: 'DELETE',
    bearerToken: token
  })
}

export function rejudgeProblem(problemId, token){
  return requestJson(`/problem/${problemId}/rejudge`, {
    method: 'POST',
    bearerToken: token
  })
}
