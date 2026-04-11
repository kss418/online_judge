import { computed, reactive, readonly } from 'vue'

import {
  canManageProblems as canManageProblemsForUser,
  canManageUsers as canManageUsersForUser,
  canRejudgeSubmissions as canRejudgeSubmissionsForUser,
  getCurrentPermissionLevel,
  isAdmin as isAdminForUser,
  isSuperAdmin as isSuperAdminForUser
} from '@/stores/auth/permissionHelper'

export const authState = reactive({
  initialized: false,
  isInitializing: false,
  isSubmitting: false,
  token: '',
  currentUser: null
})

export const state = readonly(authState)
export const currentUser = computed(() => authState.currentUser)
export const currentPermissionLevel = computed(() =>
  getCurrentPermissionLevel(authState.currentUser)
)
export const hasToken = computed(() => Boolean(authState.token))
export const isAuthenticated = computed(() => Boolean(authState.token && authState.currentUser))
export const isAdmin = computed(() => isAdminForUser(authState.currentUser))
export const isSuperAdmin = computed(() => isSuperAdminForUser(authState.currentUser))
export const canManageProblems = computed(() =>
  canManageProblemsForUser(authState.currentUser)
)
export const canManageUsers = computed(() =>
  canManageUsersForUser(authState.currentUser)
)
export const canRejudgeSubmissions = computed(() =>
  canRejudgeSubmissionsForUser(authState.currentUser)
)
