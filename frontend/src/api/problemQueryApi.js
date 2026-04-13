import { appendApiQuery } from '@/api/apiQueryBuilder'
import { requestJson } from '@/api/http'
import {
  normalizePositiveIntegerOrNull,
  normalizeTrimmedString
} from '@/api/normalizers/common'
import {
  normalizeProblemDetailResponse,
  normalizeProblemLimits,
  normalizeProblemListResponse
} from '@/api/problemResponse'

export async function getProblemList(options = {}){
  const {
    bearerToken = '',
    title = '',
    problemId = null,
    state = '',
    sort = '',
    direction = '',
    limit = null,
    offset = null
  } = options
  const path = appendApiQuery('/problem', {
    title: normalizeTrimmedString(title),
    problem_id: normalizePositiveIntegerOrNull(problemId),
    state: normalizeTrimmedString(state),
    sort: normalizeTrimmedString(sort),
    direction: normalizeTrimmedString(direction),
    limit: normalizePositiveIntegerOrNull(limit),
    offset: Number.isInteger(offset) && offset >= 0 ? offset : null
  })

  const response = await requestJson(path, {
    bearerToken
  })

  return normalizeProblemListResponse(response)
}

export async function getProblemDetail(problemId, options = {}){
  const { bearerToken = '' } = options

  const response = await requestJson(`/problem/${problemId}`, {
    bearerToken
  })

  return normalizeProblemDetailResponse(response)
}

export function getProblemLimits(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}/limits`, {
    bearerToken
  }).then(normalizeProblemLimits)
}
