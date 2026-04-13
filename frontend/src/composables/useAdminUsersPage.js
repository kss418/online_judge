import { computed, onMounted, ref, watch } from 'vue'

import { authStore } from '@/stores/auth/authStore'
import { permissionLevelToRole, useAdminUserPermissionActions } from '@/composables/adminUsers/useAdminUserPermissionActions'
import { useAdminUserSearchPagination } from '@/composables/adminUsers/useAdminUserSearchPagination'
import { useAdminUsersListResource } from '@/composables/adminUsers/useAdminUsersListResource'
import { usePollingController } from '@/composables/usePollingController'
import { formatRelativeTimestamp } from '@/utils/dateTime'

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
  const nowTimestamp = ref(Date.now())
  const userListResource = useAdminUsersListResource({
    authState,
    canManageUsers
  })
  const searchPagination = useAdminUserSearchPagination({
    users: userListResource.users
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
  const superAdminCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 2).length
  )
  const adminCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 1).length
  )
  const regularUserCount = computed(() =>
    userListResource.users.value.filter((user) => user.permission_level === 0).length
  )

  usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 1000,
    pauseWhenHidden: false,
    runImmediately: true
  })

  watch(
    [
      () => authState.initialized,
      () => authState.token,
      isAuthenticated,
      canManageUsers
    ],
    ([initialized, token, authenticated, canManage]) => {
      if (!initialized) {
        return
      }

      if (!authenticated || !canManage || !token) {
        userListResource.resetUsers()
        return
      }

      void userListResource.loadUsers()
    },
    {
      immediate: true
    }
  )

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

  function formatRelativeCreatedAt(timestamp){
    return formatRelativeTimestamp(nowTimestamp.value, timestamp)
  }

  onMounted(() => {
    void initializeAuth()
  })

  return {
    authState,
    isAuthenticated,
    canManageUsers,
    canEditPermissions,
    pageSize: searchPagination.pageSize,
    isLoading,
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
    formatRelativeCreatedAt
  }
}
