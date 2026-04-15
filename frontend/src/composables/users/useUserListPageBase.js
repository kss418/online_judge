import { computed, onMounted } from 'vue'

import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useUserSearchPagination } from '@/composables/users/useUserSearchPagination'

const defaultMessages = Object.freeze({
  loading: '유저 목록을 불러오는 중입니다.',
  emptyDefault: '표시할 사용자가 아직 없습니다.',
  emptySearch: '검색 결과가 없습니다.'
})

export function useUserListPageBase({
  users,
  isLoading,
  errorMessage,
  searchMode = 'local',
  loadUsersImpl,
  protectedAccess,
  messages = {}
}){
  const resolvedMessages = {
    ...defaultMessages,
    ...messages
  }

  function loadUsers(query){
    if (searchMode === 'remote') {
      const nextQuery = typeof query === 'string'
        ? query
        : searchPagination.appliedQuery.value

      return loadUsersImpl(nextQuery)
    }

    return loadUsersImpl()
  }

  const searchPagination = useUserSearchPagination({
    users,
    searchMode,
    onSearchSubmit(nextQuery){
      return loadUsers(nextQuery)
    },
    onSearchReset(){
      return loadUsers('')
    }
  })
  const pageAccess = protectedAccess
    ? useProtectedAdminPageAccess({
      ...protectedAccess,
      onAllowed: loadUsers
    })
    : null
  const emptyMessage = computed(() =>
    searchPagination.appliedQuery.value
      ? resolvedMessages.emptySearch
      : resolvedMessages.emptyDefault
  )
  const viewState = computed(() => {
    if (pageAccess?.accessState.value === 'initializing' || pageAccess?.accessState.value === 'logged-out') {
      return 'notice'
    }

    if (pageAccess?.accessState.value === 'denied') {
      return 'denied'
    }

    if (isLoading.value) {
      return 'loading'
    }

    if (errorMessage.value) {
      return 'error'
    }

    if (!searchPagination.filteredUsers.value.length) {
      return 'empty'
    }

    return 'ready'
  })
  const viewMessage = computed(() => {
    if (pageAccess && !pageAccess.canAccessPage.value) {
      return pageAccess.accessMessage.value
    }

    if (isLoading.value) {
      return resolvedMessages.loading
    }

    if (errorMessage.value) {
      return errorMessage.value
    }

    return ''
  })

  onMounted(() => {
    if (!pageAccess) {
      void loadUsers()
    }
  })

  return {
    pageSize: searchPagination.pageSize,
    filteredUsers: searchPagination.filteredUsers,
    pagedUsers: searchPagination.pagedUsers,
    searchInput: searchPagination.searchInput,
    appliedQuery: searchPagination.appliedQuery,
    currentPage: searchPagination.currentPage,
    totalPages: searchPagination.totalPages,
    pageJumpInput: searchPagination.pageJumpInput,
    paginationItems: searchPagination.paginationItems,
    loadUsers,
    submitSearch: searchPagination.submitSearch,
    resetSearch: searchPagination.resetSearch,
    goToPage: searchPagination.goToPage,
    submitPageJump: searchPagination.submitPageJump,
    viewState,
    viewMessage,
    emptyMessage
  }
}
