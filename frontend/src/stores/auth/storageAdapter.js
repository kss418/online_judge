export const authTokenStorageKey = 'online_judge.auth_token'

export function readToken(){
  if (typeof window === 'undefined') {
    return ''
  }

  return window.localStorage.getItem(authTokenStorageKey) || ''
}

export function writeToken(token){
  if (typeof window === 'undefined') {
    return
  }

  window.localStorage.setItem(authTokenStorageKey, token)
}

export function clearToken(){
  if (typeof window === 'undefined') {
    return
  }

  window.localStorage.removeItem(authTokenStorageKey)
}
