import { requestJson } from '@/api/http'

export function signUp(payload){
  return requestJson('/auth/sign-up', {
    method: 'POST',
    body: payload
  })
}

export function login(payload){
  return requestJson('/auth/login', {
    method: 'POST',
    body: payload
  })
}

export function logout(token){
  return requestJson('/auth/logout', {
    method: 'POST',
    bearerToken: token
  })
}

export function renewToken(token){
  return requestJson('/auth/token/renew', {
    method: 'POST',
    bearerToken: token
  })
}

export function getCurrentUser(token){
  return requestJson('/user/me', {
    bearerToken: token
  })
}
