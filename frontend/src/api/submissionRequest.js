import { appendApiQuery } from '@/api/apiQueryBuilder'
import {
  normalizePositiveIntegerOrNull,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'

export function buildSubmissionListPath(options = {}){
  return appendApiQuery('/submission', {
    before_submission_id: normalizePositiveIntegerOrNull(options.beforeSubmissionId),
    limit: normalizePositiveIntegerOrNull(options.limit),
    problem_id: normalizePositiveIntegerOrNull(options.problemId),
    user_id: normalizePositiveIntegerOrNull(options.userId),
    user_login_id: normalizeTrimmedString(options.userLoginId),
    language: normalizeTrimmedString(options.language),
    status: normalizeTrimmedString(options.status)
  })
}

export function buildSubmissionStatusBatchRequest(submissionIds){
  const normalizedSubmissionIds = Array.isArray(submissionIds)
    ? submissionIds
      .map((submissionId) => Number(submissionId))
      .filter((submissionId) => Number.isInteger(submissionId) && submissionId > 0)
    : []

  return {
    submission_ids: normalizedSubmissionIds
  }
}

export function buildCreateSubmissionBody(payload){
  return {
    language: normalizeTrimmedString(payload?.language),
    source_code: normalizeString(payload?.source_code)
  }
}
