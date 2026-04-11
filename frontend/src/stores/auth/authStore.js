import {
  canManageProblems as canManageProblemsForUser,
  canManageUsers as canManageUsersForUser,
  canRejudgeSubmissions as canRejudgeSubmissionsForUser,
  hasPermissionLevel as hasPermissionLevelForUser
} from '@/stores/auth/permissionHelper'
import {
  canManageProblems,
  canManageUsers,
  canRejudgeSubmissions,
  currentPermissionLevel,
  currentUser,
  hasToken,
  isAdmin,
  isAuthenticated,
  isSuperAdmin,
  state
} from '@/stores/auth/state'
import {
  clearSession,
  initializeSession,
  login,
  logout,
  refreshCurrentUser,
  setSession,
  signUp
} from '@/stores/auth/sessionService'

export const authStore = {
  state,
  currentUser,
  currentPermissionLevel,
  hasToken,
  isAuthenticated,
  isAdmin,
  isSuperAdmin,
  canManageProblems,
  canManageUsers,
  canRejudgeSubmissions,
  initializeAuth: initializeSession,
  initializeSession,
  setSession,
  clearSession,
  refreshCurrentUser,
  signUp,
  login,
  logout,
  hasPermissionLevel(requiredLevel){
    return hasPermissionLevelForUser(state.currentUser, requiredLevel)
  },
  canCurrentUserManageProblems(){
    return canManageProblemsForUser(state.currentUser)
  },
  canCurrentUserManageUsers(){
    return canManageUsersForUser(state.currentUser)
  },
  canCurrentUserRejudgeSubmissions(){
    return canRejudgeSubmissionsForUser(state.currentUser)
  }
}
