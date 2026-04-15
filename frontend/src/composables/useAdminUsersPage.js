import { computed } from 'vue'

import { permissionLevelToRole, useAdminUserPermissionActions } from '@/composables/adminUsers/useAdminUserPermissionActions'
import { useAdminUserListResource } from '@/composables/users/useAdminUserListResource'
import { useUserListPageBase } from '@/composables/users/useUserListPageBase'
import { authStore } from '@/stores/auth/authStore'

export function useAdminUsersPage(){
  const {
    state: authState,
    isAuthenticated,
    initializeAuth,
    refreshCurrentUser
  } = authStore
  const canManageUsers = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 2)
  const canEditPermissions = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 2)
  const currentUserId = computed(() => Number(authState.currentUser?.id ?? 0))
  const userListResource = useAdminUserListResource({
    authState,
    canLoad: canManageUsers
  })
  const isLoading = computed(() =>
    !authState.initialized || userListResource.isLoading.value
  )
  const userListPage = useUserListPageBase({
    users: userListResource.users,
    isLoading,
    errorMessage: userListResource.errorMessage,
    searchMode: 'local',
    loadUsersImpl(){
      return userListResource.loadUsers()
    },
    protectedAccess: {
      authState,
      initializeAuth,
      isAuthenticated,
      hasAccess: canManageUsers,
      onDenied: userListResource.resetUsers,
      loggedOutMessage: '권한 관리 페이지는 로그인한 슈퍼어드민만 사용할 수 있습니다.',
      deniedMessage: '이 페이지는 슈퍼어드민만 접근할 수 있습니다.'
    },
    messages: {
      emptyDefault: '표시할 사용자가 아직 없습니다.'
    }
  })
  const permissionActions = useAdminUserPermissionActions({
    authState,
    canEditPermissions,
    currentUserId,
    refreshCurrentUser,
    patchUser: userListResource.patchUser
  })
  const isBusy = computed(() =>
    authState.isInitializing || isLoading.value
  )
  const superAdminCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 2).length
  )
  const adminCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 1).length
  )
  const regularUserCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 0).length
  )

  function formatPermissionLabel(permissionLevel){
    if (permissionLevel >= permissionLevelToRole.superadmin) {
      return 'SuperAdmin'
    }

    if (permissionLevel >= permissionLevelToRole.admin) {
      return 'Admin'
    }

    return 'User'
  }

  function getPermissionTone(permissionLevel){
    if (permissionLevel >= permissionLevelToRole.superadmin) {
      return 'danger'
    }

    if (permissionLevel >= permissionLevelToRole.admin) {
      return 'warning'
    }

    return 'neutral'
  }

  return {
    authState,
    isAuthenticated,
    canManageUsers,
    canEditPermissions,
    pageSize: userListPage.pageSize,
    isLoading,
    isBusy,
    errorMessage: userListResource.errorMessage,
    users: userListResource.users,
    filteredUsers: userListPage.filteredUsers,
    pagedUsers: userListPage.pagedUsers,
    searchInput: userListPage.searchInput,
    appliedQuery: userListPage.appliedQuery,
    currentPage: userListPage.currentPage,
    totalPages: userListPage.totalPages,
    pageJumpInput: userListPage.pageJumpInput,
    paginationItems: userListPage.paginationItems,
    savingUserId: permissionActions.savingUserId,
    superAdminCount,
    adminCount,
    regularUserCount,
    loadUsers: userListPage.loadUsers,
    submitSearch: userListPage.submitSearch,
    resetSearch: userListPage.resetSearch,
    goToPage: userListPage.goToPage,
    submitPageJump: userListPage.submitPageJump,
    handlePromoteToAdmin: permissionActions.handlePromoteToAdmin,
    handleDemoteToUser: permissionActions.handleDemoteToUser,
    formatPermissionLabel,
    getPermissionTone,
    viewState: userListPage.viewState,
    viewMessage: userListPage.viewMessage,
    emptyMessage: userListPage.emptyMessage
  }
}
