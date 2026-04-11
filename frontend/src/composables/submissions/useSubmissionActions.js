import { computed, ref } from 'vue'

import { rejudgeSubmission } from '@/api/submission'
import { finishedSubmissionStatuses } from '@/composables/submissions/submissionHelpers'
import { formatApiError } from '@/utils/apiError'

export function useSubmissionActions({ authState, formatCount, patchSubmission }){
  const actionMessage = ref('')
  const actionErrorMessage = ref('')
  const rejudgingSubmissionIds = ref([])

  const canManageSubmissionRejudge = computed(() =>
    Number(authState.currentUser?.permission_level ?? 0) >= 1
  )

  function canRejudgeSubmission(submission){
    if (!canManageSubmissionRejudge.value || !authState.token) {
      return false
    }

    return finishedSubmissionStatuses.has(submission.status)
  }

  function isRejudgingSubmission(submissionId){
    return rejudgingSubmissionIds.value.includes(submissionId)
  }

  function resetRejudgingSubmissions(){
    rejudgingSubmissionIds.value = []
  }

  function addRejudgingSubmission(submissionId){
    if (isRejudgingSubmission(submissionId)) {
      return
    }

    rejudgingSubmissionIds.value = [...rejudgingSubmissionIds.value, submissionId]
  }

  function removeRejudgingSubmission(submissionId){
    rejudgingSubmissionIds.value = rejudgingSubmissionIds.value.filter(
      (queuedSubmissionId) => queuedSubmissionId !== submissionId
    )
  }

  async function handleRejudgeSubmission(submission){
    if (!authState.token || !canRejudgeSubmission(submission) || isRejudgingSubmission(submission.submission_id)) {
      return
    }

    const submissionId = submission.submission_id
    addRejudgingSubmission(submissionId)
    actionMessage.value = ''
    actionErrorMessage.value = ''

    try {
      const response = await rejudgeSubmission(submissionId, authState.token)
      patchSubmission(submissionId, {
        status: response.status || 'queued',
        elapsed_ms: null,
        max_rss_kb: null
      })
      actionMessage.value = `제출 #${formatCount(submissionId)} 재채점을 요청했습니다.`
    } catch (error) {
      actionErrorMessage.value = formatApiError(error, {
        fallback: '제출 재채점을 요청하지 못했습니다.'
      })
    } finally {
      removeRejudgingSubmission(submissionId)
    }
  }

  return {
    actionMessage,
    actionErrorMessage,
    canManageSubmissionRejudge,
    canRejudgeSubmission,
    isRejudgingSubmission,
    resetRejudgingSubmissions,
    handleRejudgeSubmission
  }
}
