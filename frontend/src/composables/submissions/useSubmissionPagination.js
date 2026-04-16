import { computed, ref } from 'vue'

import { listLimit } from '@/composables/submissions/submissionHelpers'

export function useSubmissionPagination({ submissions, formatCount }){
  const currentPage = ref(1)
  const hasMoreSubmissions = ref(false)
  const nextBeforeSubmissionId = ref(null)
  const currentBeforeSubmissionId = ref(null)
  const previousBeforeSubmissionIds = ref([])

  const submissionCount = computed(() => submissions.value.length)
  const hasPreviousPage = computed(() => previousBeforeSubmissionIds.value.length > 0)
  const visibleRangeText = computed(() => {
    if (!submissionCount.value) {
      return ''
    }

    const start = (currentPage.value - 1) * listLimit + 1
    const end = start + submissionCount.value - 1
    return `${formatCount(start)}-${formatCount(end)} 표시`
  })

  function resetPagination(){
    currentPage.value = 1
    clearNextPageAvailability()
    currentBeforeSubmissionId.value = null
    previousBeforeSubmissionIds.value = []
  }

  function clearNextPageAvailability(){
    hasMoreSubmissions.value = false
    nextBeforeSubmissionId.value = null
  }

  function applyLoadedPage({
    currentPage: nextCurrentPage,
    hasMoreSubmissions: nextHasMoreSubmissions,
    nextBeforeSubmissionId: nextNextBeforeSubmissionId,
    currentBeforeSubmissionId: nextCurrentBeforeSubmissionId,
    previousBeforeSubmissionIds: nextPreviousBeforeSubmissionIds
  }){
    currentPage.value = Number.isInteger(nextCurrentPage) && nextCurrentPage > 0
      ? nextCurrentPage
      : 1
    hasMoreSubmissions.value = Boolean(nextHasMoreSubmissions)
    nextBeforeSubmissionId.value =
      Number.isInteger(nextNextBeforeSubmissionId) && nextNextBeforeSubmissionId > 0
        ? nextNextBeforeSubmissionId
        : null
    currentBeforeSubmissionId.value =
      Number.isInteger(nextCurrentBeforeSubmissionId) && nextCurrentBeforeSubmissionId > 0
        ? nextCurrentBeforeSubmissionId
        : null
    previousBeforeSubmissionIds.value = Array.isArray(nextPreviousBeforeSubmissionIds)
      ? [...nextPreviousBeforeSubmissionIds]
      : []
  }

  function buildCurrentLoadOptions(){
    return {
      pageNumber: currentPage.value,
      beforeSubmissionId: currentBeforeSubmissionId.value,
      previousBeforeSubmissionIds: [...previousBeforeSubmissionIds.value]
    }
  }

  function buildPreviousPageLoadOptions(){
    if (!hasPreviousPage.value) {
      return null
    }

    const nextPreviousBeforeSubmissionIds = [...previousBeforeSubmissionIds.value]
    const previousBeforeSubmissionId = nextPreviousBeforeSubmissionIds.pop() ?? null

    return {
      pageNumber: Math.max(1, currentPage.value - 1),
      beforeSubmissionId: previousBeforeSubmissionId,
      previousBeforeSubmissionIds: nextPreviousBeforeSubmissionIds
    }
  }

  function buildNextPageLoadOptions(){
    if (!hasMoreSubmissions.value || !Number.isInteger(nextBeforeSubmissionId.value)) {
      return null
    }

    return {
      pageNumber: currentPage.value + 1,
      beforeSubmissionId: nextBeforeSubmissionId.value,
      previousBeforeSubmissionIds: [
        ...previousBeforeSubmissionIds.value,
        currentBeforeSubmissionId.value
      ]
    }
  }

  return {
    currentPage,
    hasMoreSubmissions,
    nextBeforeSubmissionId,
    currentBeforeSubmissionId,
    previousBeforeSubmissionIds,
    submissionCount,
    hasPreviousPage,
    visibleRangeText,
    resetPagination,
    clearNextPageAvailability,
    applyLoadedPage,
    buildCurrentLoadOptions,
    buildPreviousPageLoadOptions,
    buildNextPageLoadOptions
  }
}
