import { requestJson } from '@/api/http'
import {
  normalizeAdminUserListResponse,
  normalizeSubmissionBanResponse
} from '@/api/userResponse'

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
