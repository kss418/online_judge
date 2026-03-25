import { computed, reactive, readonly } from 'vue'

import {
  getCurrentUser,
  login as loginRequest,
  logout as logoutRequest,
  renewToken,
  signUp as signUpRequest
} from '@/api/auth'

const authTokenStorageKey = 'online_judge.auth_token'

function readStoredToken(){
  if (typeof window === 'undefined') {
    return ''
  }

  return window.localStorage.getItem(authTokenStorageKey) || ''
}

function writeStoredToken(token){
  if (typeof window === 'undefined') {
    return
  }

  window.localStorage.setItem(authTokenStorageKey, token)
}

function clearStoredToken(){
  if (typeof window === 'undefined') {
    return
  }

  window.localStorage.removeItem(authTokenStorageKey)
}

const authState = reactive({
  initialized: false,
  isInitializing: false,
  isSubmitting: false,
  token: '',
  currentUser: null
})

const isAuthenticated = computed(() => Boolean(authState.token && authState.currentUser))

let initializePromise = null

function setSession(session){
  authState.token = session.token
  authState.currentUser = {
    id: session.user_id,
    user_name: session.user_name,
    is_admin: session.is_admin
  }
  writeStoredToken(session.token)
}

function clearSession(){
  authState.token = ''
  authState.currentUser = null
  clearStoredToken()
}

async function initializeAuth(){
  if (authState.initialized) {
    return authState.currentUser
  }

  if (initializePromise) {
    return initializePromise
  }

  initializePromise = (async () => {
    authState.isInitializing = true
    authState.token = readStoredToken()

    if (!authState.token) {
      authState.initialized = true
      authState.isInitializing = false
      initializePromise = null
      return null
    }

    try {
      const currentUser = await getCurrentUser(authState.token)
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

async function signUp(payload){
  authState.isSubmitting = true

  try {
    const session = await signUpRequest(payload)
    setSession(session)
    return session
  } finally {
    authState.isSubmitting = false
  }
}

async function login(payload){
  authState.isSubmitting = true

  try {
    const session = await loginRequest(payload)
    setSession(session)
    return session
  } finally {
    authState.isSubmitting = false
  }
}

async function logout(){
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

export function useAuth(){
  return {
    authState: readonly(authState),
    isAuthenticated,
    initializeAuth,
    signUp,
    login,
    logout,
    clearSession
  }
}
