import { requestJson } from '@/api/http'
import { getRoleName, normalizePermissionLevel } from '@/api/normalizers/permission'
import {
  normalizePositiveInteger,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'

function normalizeSessionResponse(payload){
  const permissionLevel = normalizePermissionLevel(payload?.permission_level)

  return {
    token: normalizeTrimmedString(payload?.token),
    user_id: normalizePositiveInteger(payload?.user_id),
    user_login_id: normalizeTrimmedString(payload?.user_login_id),
    permission_level: permissionLevel,
    role_name: normalizeString(payload?.role_name, getRoleName(permissionLevel))
  }
}

function normalizeCurrentUserResponse(payload){
  const permissionLevel = normalizePermissionLevel(payload?.permission_level)

  return {
    user_id: normalizePositiveInteger(payload?.user_id),
    user_login_id: normalizeTrimmedString(payload?.user_login_id),
    permission_level: permissionLevel,
    role_name: normalizeString(payload?.role_name, getRoleName(permissionLevel))
  }
}

export function signUp(payload){
  return requestJson('/auth/sign-up', {
    method: 'POST',
    body: payload
  }).then(normalizeSessionResponse)
}

export function login(payload){
  return requestJson('/auth/login', {
    method: 'POST',
    body: payload
  }).then(normalizeSessionResponse)
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
  }).then(normalizeSessionResponse)
}

export function getCurrentUser(token){
  return requestJson('/user/me', {
    bearerToken: token
  }).then(normalizeCurrentUserResponse)
}
