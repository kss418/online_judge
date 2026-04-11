import { requestJson } from '@/api/http'
import { appendApiQuery } from '@/api/apiQueryBuilder'
import {
  normalizeArray,
  normalizeBoolean,
  normalizeNullableNumber,
  normalizePositiveInteger,
  normalizePositiveIntegerOrNull,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'
import { normalizeApiDateTimeField } from '@/api/normalizers/dateTime'
import { normalizeProblemStateRecord } from '@/api/normalizers/problemState'

function normalizeQueuedSubmissionResponse(response){
  return {
    submission_id: normalizePositiveInteger(response?.submission_id),
    problem_version: normalizePositiveIntegerOrNull(response?.problem_version),
    status: normalizeTrimmedString(response?.status, 'queued')
  }
}

function normalizeSubmissionListItem(submission){
  const normalizedSubmission = normalizeProblemStateRecord({
    submission_id: normalizePositiveInteger(submission?.submission_id),
    user_id: normalizePositiveInteger(submission?.user_id),
    problem_id: normalizePositiveInteger(submission?.problem_id),
    user_login_id: normalizeTrimmedString(submission?.user_login_id),
    problem_title: normalizeString(submission?.problem_title, '제목 없음'),
    language: normalizeString(submission?.language),
    status: normalizeTrimmedString(submission?.status),
    score: normalizeNullableNumber(submission?.score),
    elapsed_ms: normalizeNullableNumber(submission?.elapsed_ms),
    max_rss_kb: normalizeNullableNumber(submission?.max_rss_kb),
    user_problem_state: submission?.user_problem_state
  })

  return {
    ...normalizedSubmission,
    ...normalizeApiDateTimeField('created_at', submission?.created_at)
  }
}

function normalizeSubmissionDetailResponse(response){
  return normalizeSubmissionListItem(response)
}

function normalizeSubmissionListResponse(response){
  return {
    submission_count: normalizePositiveInteger(response?.submission_count),
    has_more: normalizeBoolean(response?.has_more),
    next_before_submission_id: normalizePositiveIntegerOrNull(response?.next_before_submission_id),
    submissions: normalizeArray(response?.submissions).map(normalizeSubmissionListItem)
  }
}

function normalizeSubmissionSourceResponse(response){
  return {
    submission_id: normalizePositiveInteger(response?.submission_id),
    language: normalizeString(response?.language),
    source_code: normalizeString(response?.source_code),
    compile_output: normalizeString(response?.compile_output),
    judge_output: normalizeString(response?.judge_output)
  }
}

function normalizeSubmissionHistoryEntry(historyEntry){
  return {
    history_id: normalizePositiveIntegerOrNull(historyEntry?.history_id),
    from_status: normalizeTrimmedString(historyEntry?.from_status) || null,
    to_status: normalizeTrimmedString(historyEntry?.to_status, 'queued'),
    reason: normalizeTrimmedString(historyEntry?.reason),
    ...normalizeApiDateTimeField('created_at', historyEntry?.created_at)
  }
}

function normalizeSubmissionHistoryResponse(response){
  return {
    histories: normalizeArray(response?.histories).map(normalizeSubmissionHistoryEntry)
  }
}

function normalizeSubmissionStatusSnapshot(statusSnapshot){
  return {
    submission_id: normalizePositiveInteger(statusSnapshot?.submission_id),
    status: normalizeTrimmedString(statusSnapshot?.status),
    score: normalizeNullableNumber(statusSnapshot?.score),
    elapsed_ms: normalizeNullableNumber(statusSnapshot?.elapsed_ms),
    max_rss_kb: normalizeNullableNumber(statusSnapshot?.max_rss_kb)
  }
}

function normalizeSubmissionStatusBatchResponse(response){
  return {
    submission_count: normalizePositiveInteger(response?.submission_count),
    submissions: normalizeArray(response?.submissions).map(normalizeSubmissionStatusSnapshot)
  }
}

export async function getSubmissionList(options = {}){
  const { bearerToken = '' } = options
  const path = appendApiQuery('/submission', {
    before_submission_id: normalizePositiveIntegerOrNull(options.beforeSubmissionId),
    limit: normalizePositiveIntegerOrNull(options.limit),
    problem_id: normalizePositiveIntegerOrNull(options.problemId),
    user_id: normalizePositiveIntegerOrNull(options.userId),
    user_login_id: normalizeTrimmedString(options.userLoginId),
    language: normalizeTrimmedString(options.language),
    status: normalizeTrimmedString(options.status)
  })

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
    body: payload,
    bearerToken: token
  }).then(normalizeQueuedSubmissionResponse)
}
