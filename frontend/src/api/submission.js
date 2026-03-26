import { requestJson } from '@/api/http'
import { normalizeProblemStateRecords } from '@/utils/problemState'

export async function getSubmissionList(options = {}){
  const { bearerToken = '' } = options
  const searchParams = new URLSearchParams()

  if (Number.isInteger(options.limit) && options.limit > 0) {
    searchParams.set('limit', String(options.limit))
  }

  if (Number.isInteger(options.problemId) && options.problemId > 0) {
    searchParams.set('problem_id', String(options.problemId))
  }

  if (Number.isInteger(options.userId) && options.userId > 0) {
    searchParams.set('user_id', String(options.userId))
  }

  if (options.status?.trim()) {
    searchParams.set('status', options.status.trim())
  }

  if (Number.isInteger(options.top) && options.top > 0) {
    searchParams.set('top', String(options.top))
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/submission?${queryString}` : '/submission'

  const response = await requestJson(path, {
    bearerToken
  })

  return {
    ...response,
    submissions: normalizeProblemStateRecords(response.submissions)
  }
}

export function getSubmissionSource(submissionId, token){
  return requestJson(`/submission/${submissionId}/source`, {
    bearerToken: token
  })
}

export function createSubmission(problemId, payload, token){
  return requestJson(`/submission/${problemId}`, {
    method: 'POST',
    body: payload,
    bearerToken: token
  })
}
