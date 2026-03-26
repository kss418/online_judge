import { requestJson } from '@/api/http'

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
