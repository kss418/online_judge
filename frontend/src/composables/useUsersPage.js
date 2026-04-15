import { computed, ref } from 'vue'

import { useUserListPageBase } from '@/composables/users/useUserListPageBase'
import { usePublicUserListResource } from '@/composables/users/usePublicUserListResource'
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
  const userListResource = usePublicUserListResource()
  const isLoading = computed(() =>
    !userListResource.hasLoadedOnce.value || userListResource.isLoading.value
  )
  const userListPage = useUserListPageBase({
    users: userListResource.users,
    isLoading,
    errorMessage: userListResource.errorMessage,
    searchMode: 'remote',
    loadUsersImpl(query){
      return userListResource.loadUsers(query)
    },
    messages: {
      emptyDefault: '표시할 유저가 아직 없습니다.'
    }
  })

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

  const formatUserCount = (value) => formatCount(value, koreanNumberFormatOptions)
  const formatUserAcceptanceRate = (acceptedSubmissionCount, submissionCount) =>
    formatAcceptanceRate(acceptedSubmissionCount, submissionCount)

  return {
    formatCount: formatUserCount,
    pageSize: userListPage.pageSize,
    users: userListResource.users,
    isLoading,
    errorMessage: userListResource.errorMessage,
    searchInput: userListPage.searchInput,
    appliedQuery: userListPage.appliedQuery,
    currentPage: userListPage.currentPage,
    totalPages: userListPage.totalPages,
    pageJumpInput: userListPage.pageJumpInput,
    paginationItems: userListPage.paginationItems,
    pagedUsers: userListPage.pagedUsers,
    loadUsers: userListPage.loadUsers,
    submitSearch: userListPage.submitSearch,
    resetSearch: userListPage.resetSearch,
    goToPage: userListPage.goToPage,
    submitPageJump: userListPage.submitPageJump,
    formatAcceptanceRate: formatUserAcceptanceRate,
    formatRelativeCreatedAt,
    viewState: userListPage.viewState,
    viewMessage: userListPage.viewMessage,
    emptyMessage: userListPage.emptyMessage
  }
}
