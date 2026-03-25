import { requestJson } from '@/api/http'

export function createSubmission(problemId, payload, token){
  return requestJson(`/submission/${problemId}`, {
    method: 'POST',
    body: payload,
    bearerToken: token
  })
}
