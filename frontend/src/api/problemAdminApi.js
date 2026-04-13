import { requestJson } from '@/api/http'
import {
  normalizeProblemCreateResponse,
  normalizeProblemVersionResponse
} from '@/api/problemResponse'

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

export function updateProblemStatement(problemId, payload, token){
  return requestJson(`/problem/${problemId}/statement`, {
    method: 'PUT',
    body: payload,
    bearerToken: token
  }).then(normalizeProblemVersionResponse)
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
