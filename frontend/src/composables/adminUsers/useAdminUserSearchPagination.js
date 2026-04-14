import { computed, ref, watch } from 'vue'

import { parsePositiveInteger } from '@/utils/parse'
import { buildPaginationItems } from '@/utils/pagination'

const defaultPageSize = 20

export function useAdminUserSearchPagination({
  users,
  pageSize = defaultPageSize
}){
  const searchInput = ref('')
  const appliedQuery = ref('')
  const currentPage = ref(1)
  const pageJumpInput = ref('')
  const filteredUsers = computed(() => {
    const keyword = appliedQuery.value.trim().toLowerCase()

    if (!keyword) {
      return users.value
    }

    return users.value.filter((user) => (
      String(user.user_login_id || '').toLowerCase().includes(keyword)
    ))
  })
  const totalPages = computed(() =>
    Math.max(1, Math.ceil(filteredUsers.value.length / pageSize))
  )
  const pagedUsers = computed(() => {
    const startIndex = (currentPage.value - 1) * pageSize
    return filteredUsers.value.slice(startIndex, startIndex + pageSize)
  })
  const paginationItems = computed(() =>
    buildPaginationItems(currentPage.value, totalPages.value)
  )

  watch(currentPage, () => {
    pageJumpInput.value = ''
  })

  watch(totalPages, (pageCount) => {
    if (currentPage.value > pageCount) {
      currentPage.value = pageCount
    }
  })

  function submitSearch(){
    appliedQuery.value = searchInput.value.trim()
    currentPage.value = 1
  }

  function resetSearch(){
    searchInput.value = ''
    appliedQuery.value = ''
    currentPage.value = 1
  }

  function goToPage(pageNumber){
    const clampedPageNumber = Math.min(Math.max(pageNumber, 1), totalPages.value)

    if (clampedPageNumber === currentPage.value) {
      return
    }

    currentPage.value = clampedPageNumber
  }

  function submitPageJump(){
    const parsedPage = parsePositiveInteger(pageJumpInput.value)
    if (parsedPage == null) {
      pageJumpInput.value = ''
      return
    }

    goToPage(parsedPage)
  }

  return {
    pageSize,
    searchInput,
    appliedQuery,
    currentPage,
    pageJumpInput,
    filteredUsers,
    totalPages,
    pagedUsers,
    paginationItems,
    submitSearch,
    resetSearch,
    goToPage,
    submitPageJump
  }
}
