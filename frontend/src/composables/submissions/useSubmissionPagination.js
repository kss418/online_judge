import { computed, ref } from 'vue'

import { listLimit } from '@/composables/submissions/submissionHelpers'

export function useSubmissionPagination({ submissions, formatCount, loadSubmissions }){
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
    hasMoreSubmissions.value = false
    nextBeforeSubmissionId.value = null
    currentBeforeSubmissionId.value = null
    previousBeforeSubmissionIds.value = []
  }

  function goToPreviousPage(){
    const nextPreviousBeforeSubmissionIds = [...previousBeforeSubmissionIds.value]
    const previousBeforeSubmissionId = nextPreviousBeforeSubmissionIds.pop() ?? null
    void loadSubmissions({
      pageNumber: Math.max(1, currentPage.value - 1),
      beforeSubmissionId: previousBeforeSubmissionId,
      previousBeforeSubmissionIds: nextPreviousBeforeSubmissionIds
    })
  }

  function goToNextPage(){
    void loadSubmissions({
      pageNumber: currentPage.value + 1,
      beforeSubmissionId: nextBeforeSubmissionId.value,
      previousBeforeSubmissionIds: [
        ...previousBeforeSubmissionIds.value,
        currentBeforeSubmissionId.value
      ]
    })
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
    goToPreviousPage,
    goToNextPage
  }
}
