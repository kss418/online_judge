import {
  getCurrentUser,
  login as loginRequest,
  logout as logoutRequest,
  renewToken,
  signUp as signUpRequest
} from '@/api/auth'
import { normalizeCurrentUser } from '@/stores/auth/permissionHelper'
import { authState } from '@/stores/auth/state'
import {
  clearToken,
  readToken,
  writeToken
} from '@/stores/auth/storageAdapter'

let initializePromise = null

export function setSession(session){
  authState.token = session.token
  authState.currentUser = normalizeCurrentUser({
    user_id: session.user_id,
    user_login_id: session.user_login_id,
    permission_level: session.permission_level,
    role_name: session.role_name
  })
  writeToken(session.token)
}

export function clearSession(){
  authState.token = ''
  authState.currentUser = null
  clearToken()
}

export async function initializeSession(){
  if (authState.initialized) {
    return authState.currentUser
  }

  if (initializePromise) {
    return initializePromise
  }

  initializePromise = (async () => {
    authState.isInitializing = true
    authState.token = readToken()

    if (!authState.token) {
      authState.initialized = true
      authState.isInitializing = false
      initializePromise = null
      return null
    }

    try {
      const currentUser = normalizeCurrentUser(await getCurrentUser(authState.token))
      authState.currentUser = currentUser

      try {
        await renewToken(authState.token)
      } catch {
        // Keep the local session when user data was already confirmed.
      }

      return currentUser
    } catch {
      clearSession()
      return null
    } finally {
      authState.initialized = true
      authState.isInitializing = false
      initializePromise = null
    }
  })()

  return initializePromise
}

export async function refreshCurrentUser(){
  if (!authState.token) {
    clearSession()
    return null
  }

  const currentUser = normalizeCurrentUser(await getCurrentUser(authState.token))
  authState.currentUser = currentUser
  return currentUser
}

export async function signUp(payload){
  authState.isSubmitting = true

  try {
    const session = await signUpRequest(payload)
    setSession(session)
    return session
  } finally {
    authState.isSubmitting = false
  }
}

export async function login(payload){
  authState.isSubmitting = true

  try {
    const session = await loginRequest(payload)
    setSession(session)
    return session
  } finally {
    authState.isSubmitting = false
  }
}

export async function logout(){
  const currentToken = authState.token
  authState.isSubmitting = true

  try {
    if (currentToken) {
      try {
        await logoutRequest(currentToken)
      } catch {
        // Clearing local auth still gives the user a predictable logout flow.
      }
    }
  } finally {
    clearSession()
    authState.isSubmitting = false
  }
}
