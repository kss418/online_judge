import { computed, onMounted, ref } from 'vue'

import { useUserListResource } from '@/composables/users/useUserListResource'
import { useUserSearchPagination } from '@/composables/users/useUserSearchPagination'
import { usePollingController } from '@/composables/usePollingController'
import { formatRelativeTimestamp } from '@/utils/dateTime'
import {
  formatAcceptanceRate,
  formatCount
} from '@/utils/numberFormat'

const koreanNumberFormatOptions = {
  locale: 'ko-KR'
}

export function useUsersPage(){
  const nowTimestamp = ref(Date.now())
  const userListResource = useUserListResource({
    mode: 'public'
  })

  function loadUsers(query = searchPagination.appliedQuery.value){
    return userListResource.loadUsers(query)
  }

  const searchPagination = useUserSearchPagination({
    users: userListResource.users,
    searchMode: 'remote',
    onSearchSubmit: loadUsers,
    onSearchReset(){
      return loadUsers('')
    }
  })
  const isLoading = computed(() =>
    !userListResource.hasLoadedOnce.value || userListResource.isLoading.value
  )
  const viewState = computed(() => {
    if (isLoading.value) {
      return 'loading'
    }

    if (userListResource.errorMessage.value) {
      return 'error'
    }

    if (!userListResource.users.value.length) {
      return 'empty'
    }

    return 'ready'
  })
  const viewMessage = computed(() => {
    if (viewState.value === 'loading') {
      return '유저 목록을 불러오는 중입니다.'
    }

    if (viewState.value === 'error') {
      return userListResource.errorMessage.value
    }

    return ''
  })
  const emptyMessage = computed(() =>
    searchPagination.appliedQuery.value
      ? '검색 결과가 없습니다.'
      : '표시할 유저가 아직 없습니다.'
  )

  usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 30_000,
    pauseWhenHidden: false,
    runImmediately: true
  })

  function formatRelativeCreatedAt(timestamp){
    return formatRelativeTimestamp(nowTimestamp.value, timestamp)
  }

  onMounted(() => {
    void loadUsers()
  })

  const formatUserCount = (value) => formatCount(value, koreanNumberFormatOptions)
  const formatUserAcceptanceRate = (acceptedSubmissionCount, submissionCount) =>
    formatAcceptanceRate(acceptedSubmissionCount, submissionCount)

  return {
    formatCount: formatUserCount,
    pageSize: searchPagination.pageSize,
    users: userListResource.users,
    isLoading,
    errorMessage: userListResource.errorMessage,
    searchInput: searchPagination.searchInput,
    appliedQuery: searchPagination.appliedQuery,
    currentPage: searchPagination.currentPage,
    totalPages: searchPagination.totalPages,
    pageJumpInput: searchPagination.pageJumpInput,
    paginationItems: searchPagination.paginationItems,
    pagedUsers: searchPagination.pagedUsers,
    loadUsers,
    submitSearch: searchPagination.submitSearch,
    resetSearch: searchPagination.resetSearch,
    goToPage: searchPagination.goToPage,
    submitPageJump: searchPagination.submitPageJump,
    formatAcceptanceRate: formatUserAcceptanceRate,
    formatRelativeCreatedAt,
    viewState,
    viewMessage,
    emptyMessage
  }
}
