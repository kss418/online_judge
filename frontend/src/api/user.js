import { requestJson } from '@/api/http'
import { appendApiQuery } from '@/api/apiQueryBuilder'
import { submissionStatusCatalog } from '@/generated/submissionStatusCatalog'
import {
  normalizeArray,
  normalizePositiveInteger,
  normalizePositiveIntegerOrNull,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'
import {
  normalizeApiDateTime,
  normalizeApiDateTimeField
} from '@/api/normalizers/dateTime'
import { getRoleName, normalizePermissionLevel } from '@/api/normalizers/permission'
import { normalizeProblemState } from '@/api/normalizers/problemState'

function normalizeSolvedProblem(problem){
  return {
    problem_id: normalizePositiveInteger(problem?.problem_id),
    accepted_count: normalizePositiveInteger(problem?.accepted_count),
    user_problem_state: normalizeProblemState(problem?.user_problem_state)
  }
}

function normalizeUserSummaryResponse(payload){
  return {
    user_id: normalizePositiveInteger(payload?.user_id),
    user_login_id: normalizeTrimmedString(payload?.user_login_id),
    ...normalizeApiDateTimeField('created_at', payload?.created_at)
  }
}

function normalizePublicUserListUser(user){
  return {
    user_id: normalizePositiveInteger(user?.user_id),
    user_login_id: normalizeTrimmedString(user?.user_login_id),
    solved_problem_count: normalizePositiveInteger(user?.solved_problem_count),
    accepted_submission_count: normalizePositiveInteger(user?.accepted_submission_count),
    submission_count: normalizePositiveInteger(user?.submission_count),
    ...normalizeApiDateTimeField('created_at', user?.created_at)
  }
}

function normalizePublicUserListResponse(response){
  return {
    users: normalizeArray(response?.users).map(normalizePublicUserListUser)
  }
}

function normalizeUserSubmissionStatisticsResponse(payload){
  const normalizedStatusCounts = Object.fromEntries(
    submissionStatusCatalog.map((statusMeta) => [
      `${statusMeta.statistics_bucket}_submission_count`,
      normalizePositiveInteger(payload?.[`${statusMeta.statistics_bucket}_submission_count`])
    ])
  )

  return {
    user_id: normalizePositiveInteger(payload?.user_id),
    submission_count: normalizePositiveInteger(payload?.submission_count),
    ...normalizedStatusCounts,
    ...normalizeApiDateTimeField('last_submission_at', payload?.last_submission_at),
    ...normalizeApiDateTimeField('last_accepted_at', payload?.last_accepted_at)
  }
}

function normalizeSolvedProblemsResponse(payload){
  return {
    solved_problems: normalizeArray(payload?.solved_problems)
      .map(normalizeSolvedProblem)
      .filter((problem) => problem.problem_id > 0)
      .sort((leftProblem, rightProblem) => leftProblem.problem_id - rightProblem.problem_id)
  }
}

function normalizeWrongProblemsResponse(payload){
  return {
    wrong_problems: normalizeArray(payload?.wrong_problems)
      .map(normalizeSolvedProblem)
      .filter((problem) => problem.problem_id > 0)
      .sort((leftProblem, rightProblem) => leftProblem.problem_id - rightProblem.problem_id)
  }
}

function normalizeSubmissionBanResponse(payload){
  const normalizedBan = normalizeApiDateTime(payload?.submission_banned_until)

  return {
    submission_banned_until: normalizedBan.value,
    submission_banned_until_timestamp: normalizedBan.timestamp,
    submission_banned_until_label: normalizedBan.label
  }
}

function normalizeAdminUser(user){
  const permissionLevel = normalizePermissionLevel(user?.permission_level)

  return {
    user_id: normalizePositiveInteger(user?.user_id),
    user_login_id: normalizeTrimmedString(user?.user_login_id),
    permission_level: permissionLevel,
    role_name: normalizeString(user?.role_name, getRoleName(permissionLevel)),
    ...normalizeApiDateTimeField('created_at', user?.created_at)
  }
}

function normalizeAdminUserListResponse(response){
  return {
    users: normalizeArray(response?.users).map(normalizeAdminUser)
  }
}

export function getPublicUserList(query = ''){
  return requestJson(appendApiQuery('/user/list', {
    q: normalizeTrimmedString(query)
  })).then(normalizePublicUserListResponse)
}

export function getUserSummary(userLoginId){
  return requestJson(`/user/id/${encodeURIComponent(userLoginId)}`)
    .then(normalizeUserSummaryResponse)
}

export function getUserSubmissionStatistics(userId){
  return requestJson(`/user/${userId}/statistics`)
    .then(normalizeUserSubmissionStatisticsResponse)
}

export function getUserSolvedProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/solved-problems`, {
    bearerToken
  }).then(normalizeSolvedProblemsResponse)
}

export function getUserWrongProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/wrong-problems`, {
    bearerToken
  }).then(normalizeWrongProblemsResponse)
}

export function getMySubmissionBan(token){
  return requestJson('/user/me/submission-ban', {
    bearerToken: token
  }).then(normalizeSubmissionBanResponse)
}

export function getUserList(token){
  return requestJson('/user', {
    bearerToken: token
  }).then(normalizeAdminUserListResponse)
}

export function promoteUserToAdmin(userId, token){
  return requestJson(`/user/${userId}/admin`, {
    method: 'PUT',
    bearerToken: token
  })
}

export function demoteUserToUser(userId, token){
  return requestJson(`/user/${userId}/user`, {
    method: 'PUT',
    bearerToken: token
  })
}

export function createUserSubmissionBan(userId, durationMinutes, token){
  return requestJson(`/user/${userId}/submission-ban`, {
    method: 'POST',
    bearerToken: token,
    body: {
      duration_minutes: durationMinutes
    }
  }).then(normalizeSubmissionBanResponse)
}

export function getUserSubmissionBan(userId, token){
  return requestJson(`/user/${userId}/submission-ban`, {
    bearerToken: token
  }).then(normalizeSubmissionBanResponse)
}

export function clearUserSubmissionBan(userId, token){
  return requestJson(`/user/${userId}/submission-ban`, {
    method: 'DELETE',
    bearerToken: token
  }).then(normalizeSubmissionBanResponse)
}
