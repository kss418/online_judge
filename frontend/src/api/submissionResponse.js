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

export function normalizeQueuedSubmissionResponse(response){
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

export function normalizeSubmissionDetailResponse(response){
  return normalizeSubmissionListItem(response)
}

export function normalizeSubmissionListResponse(response){
  return {
    submission_count: normalizePositiveInteger(response?.submission_count),
    has_more: normalizeBoolean(response?.has_more),
    next_before_submission_id: normalizePositiveIntegerOrNull(response?.next_before_submission_id),
    submissions: normalizeArray(response?.submissions).map(normalizeSubmissionListItem)
  }
}

export function normalizeSubmissionSourceResponse(response){
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

export function normalizeSubmissionHistoryResponse(response){
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

export function normalizeSubmissionStatusBatchResponse(response){
  return {
    submission_count: normalizePositiveInteger(response?.submission_count),
    submissions: normalizeArray(response?.submissions).map(normalizeSubmissionStatusSnapshot)
  }
}
