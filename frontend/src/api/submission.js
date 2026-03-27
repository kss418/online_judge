import { requestJson } from '@/api/http'
import { normalizeProblemStateRecords } from '@/utils/problemState'

export async function getSubmissionList(options = {}){
  const { bearerToken = '' } = options
  const searchParams = new URLSearchParams()

  if (Number.isInteger(options.page) && options.page > 0) {
    searchParams.set('page', String(options.page))
  }

  if (Number.isInteger(options.limit) && options.limit > 0) {
    searchParams.set('limit', String(options.limit))
  }

  if (Number.isInteger(options.problemId) && options.problemId > 0) {
    searchParams.set('problem_id', String(options.problemId))
  }

  if (Number.isInteger(options.userId) && options.userId > 0) {
    searchParams.set('user_id', String(options.userId))
  }

  if (options.language?.trim()) {
    searchParams.set('language', options.language.trim())
  }

  if (options.status?.trim()) {
    searchParams.set('status', options.status.trim())
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/submission?${queryString}` : '/submission'

  const response = await requestJson(path, {
    bearerToken
  })

  return {
    ...response,
    submission_count: Number(response.submission_count ?? 0),
    total_submission_count: Number(
      response.total_submission_count ?? response.submission_count ?? 0
    ),
    submissions: normalizeProblemStateRecords(response.submissions)
  }
}

export function getSubmissionSource(submissionId, token){
  return requestJson(`/submission/${submissionId}/source`, {
    bearerToken: token
  })
}

export function getSubmissionDetail(submissionId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/submission/${submissionId}`, {
    bearerToken
  })
}

export function rejudgeSubmission(submissionId, token){
  return requestJson(`/submission/${submissionId}/rejudge`, {
    method: 'POST',
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
