import { requestJson } from '@/api/http'
import { normalizeProblemStateRecords } from '@/utils/problemState'

export async function getSubmissionList(options = {}){
  const { bearerToken = '' } = options
  const searchParams = new URLSearchParams()

  if (Number.isInteger(options.beforeSubmissionId) && options.beforeSubmissionId > 0) {
    searchParams.set('before_submission_id', String(options.beforeSubmissionId))
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

  if (options.userLoginId?.trim()) {
    searchParams.set('user_login_id', options.userLoginId.trim())
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
    has_more: Boolean(response.has_more),
    next_before_submission_id: Number.isInteger(Number(response.next_before_submission_id))
      && Number(response.next_before_submission_id) > 0
      ? Number(response.next_before_submission_id)
      : null,
    submissions: normalizeProblemStateRecords(response.submissions)
  }
}

export function getSubmissionSource(submissionId, token){
  return requestJson(`/submission/${submissionId}/source`, {
    bearerToken: token
  })
}

export function getSubmissionHistory(submissionId, token){
  return requestJson(`/submission/${submissionId}/history`, {
    bearerToken: token
  })
}

export function getSubmissionDetail(submissionId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/submission/${submissionId}`, {
    bearerToken
  })
}

export async function getSubmissionStatusBatch(submissionIds, options = {}){
  const { bearerToken = '' } = options
  const normalizedSubmissionIds = Array.isArray(submissionIds)
    ? submissionIds
      .map((submissionId) => Number(submissionId))
      .filter((submissionId) => Number.isInteger(submissionId) && submissionId > 0)
    : []

  if (normalizedSubmissionIds.length === 0) {
    return {
      submission_count: 0,
      submissions: []
    }
  }

  const response = await requestJson('/submission/status/batch', {
    method: 'POST',
    body: {
      submission_ids: normalizedSubmissionIds
    },
    bearerToken
  })

  return {
    submission_count: Number(response.submission_count ?? 0),
    submissions: Array.isArray(response.submissions) ? response.submissions : []
  }
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
