import { computed, onMounted, ref, watch } from 'vue'

import { getPublicUserList } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { usePollingController } from '@/composables/usePollingController'
import { formatApiError } from '@/utils/apiError'
import { formatRelativeTimestamp } from '@/utils/dateTime'
import {
  formatAcceptanceRate,
  formatCount
} from '@/utils/numberFormat'
import { buildPaginationItems } from '@/utils/pagination'

const pageSize = 20
const koreanNumberFormatOptions = {
  locale: 'ko-KR'
}

export function useUsersPage(){
  const searchInput = ref('')
  const appliedQuery = ref('')
  const currentPage = ref(1)
  const pageJumpInput = ref('')
  const nowTimestamp = ref(Date.now())
  const userListResource = useAsyncResource({
    initialData: () => [],
    async load({ query }){
      const response = await getPublicUserList(query)
      return Array.isArray(response.users) ? response.users : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '유저 목록을 불러오지 못했습니다.'
      })
    }
  })
  const users = userListResource.data
  const isLoading = computed(() =>
    !userListResource.hasLoadedOnce.value || userListResource.isLoading.value
  )
  const totalPages = computed(() =>
    Math.max(1, Math.ceil(users.value.length / pageSize))
  )
  const pagedUsers = computed(() => {
    const startIndex = (currentPage.value - 1) * pageSize
    return users.value.slice(startIndex, startIndex + pageSize)
  })
  const paginationItems = computed(() =>
    buildPaginationItems(currentPage.value, totalPages.value)
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

  watch(currentPage, () => {
    pageJumpInput.value = ''
  })

  watch(totalPages, (pageCount) => {
    if (currentPage.value > pageCount) {
      currentPage.value = pageCount
    }
  })

  function formatRelativeCreatedAt(timestamp){
    return formatRelativeTimestamp(nowTimestamp.value, timestamp)
  }

  async function loadUsers(){
    return userListResource.run({
      query: appliedQuery.value
    }, {
      resetDataOnError: true
    })
  }

  function submitSearch(){
    appliedQuery.value = searchInput.value.trim()
    currentPage.value = 1
    void loadUsers()
  }

  function resetSearch(){
    searchInput.value = ''
    appliedQuery.value = ''
    currentPage.value = 1
    void loadUsers()
  }

  function goToPage(pageNumber){
    const clampedPageNumber = Math.min(Math.max(pageNumber, 1), totalPages.value)

    if (clampedPageNumber === currentPage.value) {
      return
    }

    currentPage.value = clampedPageNumber
  }

  function submitPageJump(){
    const parsedPage = Number.parseInt(pageJumpInput.value, 10)

    if (!Number.isInteger(parsedPage)) {
      pageJumpInput.value = ''
      return
    }

    goToPage(parsedPage)
  }

  onMounted(() => {
    void loadUsers()
  })

  const formatUserCount = (value) => formatCount(value, koreanNumberFormatOptions)
  const formatUserAcceptanceRate = (acceptedSubmissionCount, submissionCount) =>
    formatAcceptanceRate(acceptedSubmissionCount, submissionCount)

  return {
    formatCount: formatUserCount,
    pageSize,
    users,
    isLoading,
    errorMessage: userListResource.errorMessage,
    searchInput,
    appliedQuery,
    currentPage,
    totalPages,
    pageJumpInput,
    paginationItems,
    pagedUsers,
    loadUsers,
    submitSearch,
    resetSearch,
    goToPage,
    submitPageJump,
    formatAcceptanceRate: formatUserAcceptanceRate,
    formatRelativeCreatedAt
  }
}
