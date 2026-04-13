import { requestJson } from '@/api/http'
import {
  normalizeProblemTestcaseResponse,
  normalizeProblemTestcasesResponse,
  normalizeProblemTestcaseZipResponse,
  normalizeProblemVersionResponse
} from '@/api/problemResponse'

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
