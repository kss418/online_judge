import { computed, onMounted, onUnmounted, ref } from 'vue'

import { getSubmissionStatusBatch } from '@/api/submission'
import {
  formatRelativeSubmittedAt,
  pollingSubmissionStatuses,
  submissionPollingIntervalMs
} from '@/composables/submissions/submissionHelpers'

export function useSubmissionPolling({
  authState,
  authenticatedBearerToken,
  canManageSubmissionRejudge,
  hasAppliedStatusFilter,
  isLoading,
  errorMessage,
  submissions,
  mergeSubmissionStatusBatch,
  historyDialogOpen,
  isLoadingHistory,
  historyErrorMessage,
  activeHistorySubmissionId,
  latestHistoryStatus,
  fetchSubmissionHistory
}){
  const nowTimestamp = ref(Date.now())
  const isDocumentVisible = ref(typeof document === 'undefined' ? true : !document.hidden)

  const pollingSubmissionIds = computed(() =>
    submissions.value
      .filter((submission) => pollingSubmissionStatuses.has(submission.status))
      .map((submission) => submission.submission_id)
  )
  const shouldPollSubmissionHistory = computed(() =>
    historyDialogOpen.value &&
    !isLoadingHistory.value &&
    !historyErrorMessage.value &&
    Number.isInteger(activeHistorySubmissionId.value) &&
    isDocumentVisible.value &&
    Boolean(authState.token) &&
    canManageSubmissionRejudge.value &&
    pollingSubmissionStatuses.has(latestHistoryStatus.value)
  )

  let relativeTimeRefreshTimer = null
  let submissionPollingTimer = null
  let isPollingSubmissionDetails = false
  let historyPollingTimer = null
  let isPollingSubmissionHistory = false

  function formatRelativeSubmitted(timestamp){
    return formatRelativeSubmittedAt(nowTimestamp.value, timestamp)
  }

  function shouldPollSubmissions(){
    return !isLoading.value &&
      !errorMessage.value &&
      isDocumentVisible.value &&
      !hasAppliedStatusFilter.value &&
      pollingSubmissionIds.value.length > 0
  }

  function stopHistoryPolling(){
    if (historyPollingTimer) {
      clearTimeout(historyPollingTimer)
      historyPollingTimer = null
    }
  }

  function startHistoryPolling(){
    if (
      typeof window === 'undefined' ||
      historyPollingTimer ||
      isPollingSubmissionHistory ||
      !shouldPollSubmissionHistory.value
    ) {
      return
    }

    historyPollingTimer = window.setTimeout(() => {
      historyPollingTimer = null
      void pollSubmissionHistory()
    }, submissionPollingIntervalMs)
  }

  function syncHistoryPolling(){
    if (!shouldPollSubmissionHistory.value) {
      stopHistoryPolling()
      return
    }

    startHistoryPolling()
  }

  function stopSubmissionPolling(){
    if (submissionPollingTimer) {
      clearTimeout(submissionPollingTimer)
      submissionPollingTimer = null
    }
  }

  function startSubmissionPolling(){
    if (
      typeof window === 'undefined' ||
      submissionPollingTimer ||
      isPollingSubmissionDetails ||
      !shouldPollSubmissions()
    ) {
      return
    }

    submissionPollingTimer = window.setTimeout(() => {
      submissionPollingTimer = null
      void pollActiveSubmissions()
    }, submissionPollingIntervalMs)
  }

  function syncSubmissionPolling(){
    if (!shouldPollSubmissions()) {
      stopSubmissionPolling()
      return
    }

    startSubmissionPolling()
  }

  async function pollActiveSubmissions(){
    if (isPollingSubmissionDetails || !shouldPollSubmissions()) {
      return
    }

    isPollingSubmissionDetails = true
    const activeSubmissionIds = [...new Set(pollingSubmissionIds.value)]

    try {
      const response = await getSubmissionStatusBatch(activeSubmissionIds, {
        bearerToken: authenticatedBearerToken.value
      })
      mergeSubmissionStatusBatch(
        Array.isArray(response.submissions) ? response.submissions : []
      )
    } catch {
    } finally {
      isPollingSubmissionDetails = false
      syncSubmissionPolling()
    }
  }

  function handleDocumentVisibilityChange(){
    if (typeof document === 'undefined') {
      isDocumentVisible.value = true
    } else {
      isDocumentVisible.value = !document.hidden
    }

    syncSubmissionPolling()
  }

  async function pollSubmissionHistory(){
    if (
      isPollingSubmissionHistory ||
      !shouldPollSubmissionHistory.value ||
      !Number.isInteger(activeHistorySubmissionId.value)
    ) {
      return
    }

    isPollingSubmissionHistory = true

    try {
      await fetchSubmissionHistory(activeHistorySubmissionId.value, {
        background: true
      })
    } finally {
      isPollingSubmissionHistory = false
      syncHistoryPolling()
    }
  }

  function stopRelativeTimeRefresh(){
    if (relativeTimeRefreshTimer) {
      clearInterval(relativeTimeRefreshTimer)
      relativeTimeRefreshTimer = null
    }
  }

  function startRelativeTimeRefresh(){
    stopRelativeTimeRefresh()
    nowTimestamp.value = Date.now()
    relativeTimeRefreshTimer = window.setInterval(() => {
      nowTimestamp.value = Date.now()
    }, 1000)
  }

  onMounted(() => {
    startRelativeTimeRefresh()
    handleDocumentVisibilityChange()

    if (typeof document !== 'undefined') {
      document.addEventListener('visibilitychange', handleDocumentVisibilityChange)
    }
  })

  onUnmounted(() => {
    stopHistoryPolling()
    stopSubmissionPolling()
    stopRelativeTimeRefresh()

    if (typeof document !== 'undefined') {
      document.removeEventListener('visibilitychange', handleDocumentVisibilityChange)
    }
  })

  return {
    isDocumentVisible,
    pollingSubmissionIds,
    shouldPollSubmissionHistory,
    formatRelativeSubmitted: formatRelativeSubmitted,
    syncHistoryPolling,
    syncSubmissionPolling
  }
}
