import { requestJson } from '@/api/http'
import {
  buildCreateSubmissionBody,
  buildSubmissionListPath,
  buildSubmissionStatusBatchRequest
} from '@/api/submissionRequest'
import {
  normalizeQueuedSubmissionResponse,
  normalizeSubmissionDetailResponse,
  normalizeSubmissionHistoryResponse,
  normalizeSubmissionListResponse,
  normalizeSubmissionSourceResponse,
  normalizeSubmissionStatusBatchResponse
} from '@/api/submissionResponse'

export async function getSubmissionList(options = {}){
  const { bearerToken = '' } = options
  const path = buildSubmissionListPath(options)

  const response = await requestJson(path, {
    bearerToken
  })

  return normalizeSubmissionListResponse(response)
}

export function getSubmissionSource(submissionId, token){
  return requestJson(`/submission/${submissionId}/source`, {
    bearerToken: token
  }).then(normalizeSubmissionSourceResponse)
}

export function getSubmissionHistory(submissionId, token){
  return requestJson(`/submission/${submissionId}/history`, {
    bearerToken: token
  }).then(normalizeSubmissionHistoryResponse)
}

export function getSubmissionDetail(submissionId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/submission/${submissionId}`, {
    bearerToken
  }).then(normalizeSubmissionDetailResponse)
}

export async function getSubmissionStatusBatch(submissionIds, options = {}){
  const { bearerToken = '' } = options
  const requestBody = buildSubmissionStatusBatchRequest(submissionIds)

  if (requestBody.submission_ids.length === 0) {
    return {
      submission_count: 0,
      submissions: []
    }
  }

  const response = await requestJson('/submission/status/batch', {
    method: 'POST',
    body: requestBody,
    bearerToken
  })

  return normalizeSubmissionStatusBatchResponse(response)
}

export function rejudgeSubmission(submissionId, token){
  return requestJson(`/submission/${submissionId}/rejudge`, {
    method: 'POST',
    bearerToken: token
  }).then(normalizeQueuedSubmissionResponse)
}

export function createSubmission(problemId, payload, token){
  return requestJson(`/submission/${problemId}`, {
    method: 'POST',
    body: buildCreateSubmissionBody(payload),
    bearerToken: token
  }).then(normalizeQueuedSubmissionResponse)
}
