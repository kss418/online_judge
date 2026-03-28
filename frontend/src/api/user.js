import { requestJson } from '@/api/http'

export function getPublicUserList(query = ''){
  const searchParams = new URLSearchParams()
  if (typeof query === 'string' && query.trim()) {
    searchParams.set('q', query.trim())
  }

  const suffix = searchParams.size > 0 ? `?${searchParams.toString()}` : ''
  return requestJson(`/user/list${suffix}`)
}

export function getUserSummary(userLoginId){
  return requestJson(`/user/id/${encodeURIComponent(userLoginId)}`)
}

export function getUserSubmissionStatistics(userId){
  return requestJson(`/user/${userId}/statistics`)
}

export function getUserSolvedProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/solved-problems`, {
    bearerToken
  })
}

export function getUserWrongProblems(userId, bearerToken){
  return requestJson(`/user/${userId}/wrong-problems`, {
    bearerToken
  })
}

export function getUserList(token){
  return requestJson('/user', {
    bearerToken: token
  })
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
