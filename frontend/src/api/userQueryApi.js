import { appendApiQuery } from '@/api/apiQueryBuilder'
import { requestJson } from '@/api/http'
import { normalizeTrimmedString } from '@/api/normalizers/common'
import {
  normalizePublicUserListResponse,
  normalizeSubmissionBanResponse,
  normalizeUserSummaryResponse
} from '@/api/userResponse'

export function getPublicUserList(query = ''){
  return requestJson(appendApiQuery('/user/list', {
    q: normalizeTrimmedString(query)
  })).then(normalizePublicUserListResponse)
}

export function getUserSummary(userLoginId){
  return requestJson(`/user/id/${encodeURIComponent(userLoginId)}`)
    .then(normalizeUserSummaryResponse)
}

export function getMySubmissionBan(token){
  return requestJson('/user/me/submission-ban', {
    bearerToken: token
  }).then(normalizeSubmissionBanResponse)
}
