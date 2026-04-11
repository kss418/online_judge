import { computed, ref } from 'vue'

import { getSubmissionStatusBatch } from '@/api/submission'
import { usePollingController } from '@/composables/usePollingController'
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
  const pollingSubmissionIds = computed(() =>
    submissions.value
      .filter((submission) => pollingSubmissionStatuses.has(submission.status))
      .map((submission) => submission.submission_id)
  )
  const shouldPollSubmissions = computed(() =>
    !isLoading.value &&
    !errorMessage.value &&
    !hasAppliedStatusFilter.value &&
    pollingSubmissionIds.value.length > 0
  )
  const historyPollingEnabled = computed(() =>
    historyDialogOpen.value &&
    !isLoadingHistory.value &&
    !historyErrorMessage.value &&
    Number.isInteger(activeHistorySubmissionId.value) &&
    Boolean(authState.token) &&
    canManageSubmissionRejudge.value &&
    pollingSubmissionStatuses.has(latestHistoryStatus.value)
  )
  const relativeTimeController = usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 1000,
    pauseWhenHidden: false,
    runImmediately: true
  })
  const activeSubmissionPollingController = usePollingController({
    async task(){
      const activeSubmissionIds = [...new Set(pollingSubmissionIds.value)]

      if (!activeSubmissionIds.length) {
        return
      }

      const response = await getSubmissionStatusBatch(activeSubmissionIds, {
        bearerToken: authenticatedBearerToken.value
      })
      mergeSubmissionStatusBatch(
        Array.isArray(response.submissions) ? response.submissions : []
      )
    },
    enabled: shouldPollSubmissions,
    intervalMs: submissionPollingIntervalMs
  })
  const historyPollingController = usePollingController({
    async task(){
      if (!Number.isInteger(activeHistorySubmissionId.value)) {
        return
      }

      await fetchSubmissionHistory(activeHistorySubmissionId.value, {
        background: true
      })
    },
    enabled: historyPollingEnabled,
    intervalMs: submissionPollingIntervalMs
  })
  const shouldPollSubmissionHistory = computed(() =>
    historyPollingEnabled.value && historyPollingController.isRunning.value
  )

  function formatRelativeSubmitted(timestamp){
    return formatRelativeSubmittedAt(nowTimestamp.value, timestamp)
  }

  return {
    shouldPollSubmissionHistory,
    formatRelativeSubmitted
  }
}
