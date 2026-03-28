import { requestJson } from '@/api/http'

export function getUserList(token){
  return requestJson('/user', {
    bearerToken: token
  })
}

export function getMySubmissionStatistics(token){
  return requestJson('/user/me/statistics', {
    bearerToken: token
  })
}

export function getMySolvedProblems(token){
  return requestJson('/user/me/solved-problems', {
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
