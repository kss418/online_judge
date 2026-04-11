import { authStore } from '@/stores/auth/authStore'

const authComposable = {
  authState: authStore.state,
  isAuthenticated: authStore.isAuthenticated,
  initializeAuth: authStore.initializeAuth,
  signUp: authStore.signUp,
  login: authStore.login,
  logout: authStore.logout,
  refreshCurrentUser: authStore.refreshCurrentUser,
  clearSession: authStore.clearSession
}

export function useAuth(){
  return authComposable
}
