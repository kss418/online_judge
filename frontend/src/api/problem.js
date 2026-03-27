import { requestJson } from '@/api/http'
import {
  normalizeProblemStateRecord,
  normalizeProblemStateRecords
} from '@/utils/problemState'

export async function getProblemList(options = {}){
  const { bearerToken = '', title = '' } = options
  const searchParams = new URLSearchParams()

  if (title?.trim()) {
    searchParams.set('title', title.trim())
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/problem?${queryString}` : '/problem'

  const response = await requestJson(path, {
    bearerToken
  })

  return {
    ...response,
    problems: normalizeProblemStateRecords(response.problems)
  }
}

export async function getProblemDetail(problemId, options = {}){
  const { bearerToken = '' } = options

  const response = await requestJson(`/problem/${problemId}`, {
    bearerToken
  })

  return normalizeProblemStateRecord(response)
}

export function createProblem(payload, token){
  return requestJson('/problem', {
    method: 'POST',
    body: payload,
    bearerToken: token
  })
}

export function updateProblemTitle(problemId, payload, token){
  return requestJson(`/problem/${problemId}/title`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  })
}

export function updateProblemLimits(problemId, payload, token){
  return requestJson(`/problem/${problemId}/limits`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  })
}

export function updateProblemStatement(problemId, payload, token){
  return requestJson(`/problem/${problemId}/statement`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  })
}

export function getProblemSamples(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/sample`, {
    bearerToken
  })
}

export function createProblemSample(problemId, token){
  return requestJson(`/problem/${problemId}/sample`, {
    method: 'POST',
    bearerToken: token
  })
}

export function updateProblemSample(problemId, sampleOrder, payload, token){
  return requestJson(`/problem/${problemId}/sample/${sampleOrder}`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  })
}

export function deleteProblemSample(problemId, token){
  return requestJson(`/problem/${problemId}/sample`, {
    method: 'DELETE',
    bearerToken: token
  })
}

export function uploadProblemTestcaseZip(problemId, file, token){
  return requestJson(`/problem/${problemId}/testcase/zip`, {
    method: 'POST',
    body: file,
    bearerToken: token,
    headers: {
      'Content-Type': 'application/zip'
    }
  })
}

export function getProblemTestcases(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/testcase`, {
    bearerToken
  })
}

export function createProblemTestcase(problemId, payload, token){
  return requestJson(`/problem/${problemId}/testcase`, {
    method: 'POST',
    body: payload,
    bearerToken: token
  })
}

export function deleteProblemTestcase(problemId, token){
  return requestJson(`/problem/${problemId}/testcase`, {
    method: 'DELETE',
    bearerToken: token
  })
}

export function updateProblemTestcase(problemId, testcaseOrder, payload, token){
  return requestJson(`/problem/${problemId}/testcase/${testcaseOrder}`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  })
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
