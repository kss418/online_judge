import { requestJson } from '@/api/http'
import {
  normalizeProblemSampleResponse,
  normalizeProblemSamplesResponse,
  normalizeProblemVersionResponse
} from '@/api/problemResponse'

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
