import { computed } from 'vue'

import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { permissionLevelToRole, useAdminUserPermissionActions } from '@/composables/adminUsers/useAdminUserPermissionActions'
import { useUserListResource } from '@/composables/users/useUserListResource'
import { useUserSearchPagination } from '@/composables/users/useUserSearchPagination'
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
  const userListResource = useUserListResource({
    mode: 'admin',
    authState,
    canLoad: canManageUsers
  })
  const searchPagination = useUserSearchPagination({
    users: userListResource.users,
    searchMode: 'local'
  })
  const permissionActions = useAdminUserPermissionActions({
    authState,
    canEditPermissions,
    currentUserId,
    refreshCurrentUser,
    patchUser: userListResource.patchUser
  })
  const isLoading = computed(() =>
    !authState.initialized || userListResource.isLoading.value
  )
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
  const emptyMessage = computed(() =>
    searchPagination.appliedQuery.value
      ? '검색 결과가 없습니다.'
      : '표시할 사용자가 아직 없습니다.'
  )
  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageUsers,
    onDenied: userListResource.resetUsers,
    onAllowed: userListResource.loadUsers,
    loggedOutMessage: '권한 관리 페이지는 로그인한 슈퍼어드민만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 슈퍼어드민만 접근할 수 있습니다.'
  })
  const viewState = computed(() => {
    if (pageAccess.accessState.value === 'initializing' || pageAccess.accessState.value === 'logged-out') {
      return 'notice'
    }

    if (pageAccess.accessState.value === 'denied') {
      return 'denied'
    }

    if (isLoading.value) {
      return 'loading'
    }

    if (userListResource.errorMessage.value) {
      return 'error'
    }

    if (!searchPagination.filteredUsers.value.length) {
      return 'empty'
    }

    return 'ready'
  })
  const viewMessage = computed(() => {
    if (!pageAccess.canAccessPage.value) {
      return pageAccess.accessMessage.value
    }

    if (isLoading.value) {
      return '유저 목록을 불러오는 중입니다.'
    }

    if (userListResource.errorMessage.value) {
      return userListResource.errorMessage.value
    }

    return ''
  })

  function loadUsers(){
    return userListResource.loadUsers()
  }

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
    pageSize: searchPagination.pageSize,
    isLoading,
    isBusy,
    errorMessage: userListResource.errorMessage,
    users: userListResource.users,
    filteredUsers: searchPagination.filteredUsers,
    pagedUsers: searchPagination.pagedUsers,
    searchInput: searchPagination.searchInput,
    appliedQuery: searchPagination.appliedQuery,
    currentPage: searchPagination.currentPage,
    totalPages: searchPagination.totalPages,
    pageJumpInput: searchPagination.pageJumpInput,
    paginationItems: searchPagination.paginationItems,
    savingUserId: permissionActions.savingUserId,
    superAdminCount,
    adminCount,
    regularUserCount,
    loadUsers,
    submitSearch: searchPagination.submitSearch,
    resetSearch: searchPagination.resetSearch,
    goToPage: searchPagination.goToPage,
    submitPageJump: searchPagination.submitPageJump,
    handlePromoteToAdmin: permissionActions.handlePromoteToAdmin,
    handleDemoteToUser: permissionActions.handleDemoteToUser,
    formatPermissionLabel,
    getPermissionTone,
    viewState,
    viewMessage,
    emptyMessage
  }
}
