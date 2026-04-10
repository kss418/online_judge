import { computed, onUnmounted, ref } from 'vue'

import {
  getSubmissionHistory,
  getSubmissionSource
} from '@/api/submission'
import {
  fallbackCopyText,
  formatHistoryTransition,
  normalizeSubmissionHistoryEntry
} from '@/composables/submissions/submissionHelpers'

export function useSubmissionDialogs({
  authState,
  canManageSubmissionRejudge,
  patchSubmission
}){
  const historyDialogOpen = ref(false)
  const isLoadingHistory = ref(false)
  const historyErrorMessage = ref('')
  const submissionHistoryEntries = ref([])
  const activeHistorySubmissionId = ref(null)
  const sourceDialogOpen = ref(false)
  const isLoadingSource = ref(false)
  const sourceErrorMessage = ref('')
  const sourceDetail = ref(null)
  const activeSourceSubmissionId = ref(null)
  const copyState = ref('idle')

  const copyButtonLabel = computed(() => {
    if (copyState.value === 'success') {
      return '복사됨'
    }

    if (copyState.value === 'error') {
      return '복사 실패'
    }

    return '복사'
  })
  const latestHistoryStatus = computed(() => {
    const latestHistoryEntry = submissionHistoryEntries.value[
      submissionHistoryEntries.value.length - 1
    ]

    return latestHistoryEntry?.to_status || ''
  })

  let copyStateResetTimer = null
  let latestHistoryLoadRequestId = 0

  function canViewSource(submission){
    if (!authState.currentUser) {
      return false
    }

    if (Number(authState.currentUser.permission_level ?? 0) >= 1) {
      return true
    }

    return Number(submission.user_id) === Number(authState.currentUser.id)
  }

  function resetCopyState(){
    copyState.value = 'idle'

    if (copyStateResetTimer) {
      clearTimeout(copyStateResetTimer)
      copyStateResetTimer = null
    }
  }

  function scheduleCopyStateReset(){
    if (copyStateResetTimer) {
      clearTimeout(copyStateResetTimer)
    }

    copyStateResetTimer = window.setTimeout(() => {
      copyState.value = 'idle'
      copyStateResetTimer = null
    }, 1400)
  }

  function closeHistoryDialog(){
    latestHistoryLoadRequestId += 1
    historyDialogOpen.value = false
    isLoadingHistory.value = false
    historyErrorMessage.value = ''
    submissionHistoryEntries.value = []
    activeHistorySubmissionId.value = null
  }

  async function fetchSubmissionHistory(submissionId, options = {}){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    const { background = false } = options
    const requestId = ++latestHistoryLoadRequestId

    if (!background) {
      isLoadingHistory.value = true
      historyErrorMessage.value = ''
      submissionHistoryEntries.value = []
    }

    try {
      const response = await getSubmissionHistory(submissionId, authState.token)

      if (
        requestId !== latestHistoryLoadRequestId ||
        activeHistorySubmissionId.value !== submissionId
      ) {
        return
      }

      const normalizedHistories = Array.isArray(response.histories)
        ? response.histories.map((historyEntry, index) =>
          normalizeSubmissionHistoryEntry(historyEntry, index)
        )
        : []

      submissionHistoryEntries.value = normalizedHistories

      const latestHistoryEntry = normalizedHistories[normalizedHistories.length - 1]
      if (latestHistoryEntry) {
        patchSubmission(submissionId, {
          status: latestHistoryEntry.to_status
        })
      }
    } catch (error) {
      if (
        requestId !== latestHistoryLoadRequestId ||
        activeHistorySubmissionId.value !== submissionId
      ) {
        return
      }

      if (!background) {
        historyErrorMessage.value = error instanceof Error
          ? error.message
          : '채점 내역을 불러오지 못했습니다.'
        submissionHistoryEntries.value = []
      }
    } finally {
      if (
        !background &&
        requestId === latestHistoryLoadRequestId &&
        activeHistorySubmissionId.value === submissionId
      ) {
        isLoadingHistory.value = false
      }
    }
  }

  async function openHistoryDialog(submission){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    historyDialogOpen.value = true
    isLoadingHistory.value = true
    historyErrorMessage.value = ''
    submissionHistoryEntries.value = []
    activeHistorySubmissionId.value = submission.submission_id

    await fetchSubmissionHistory(submission.submission_id)
  }

  function closeSourceDialog(){
    sourceDialogOpen.value = false
    isLoadingSource.value = false
    sourceErrorMessage.value = ''
    sourceDetail.value = null
    activeSourceSubmissionId.value = null
    resetCopyState()
  }

  async function openSourceDialog(submission){
    if (!canViewSource(submission) || !authState.token) {
      return
    }

    sourceDialogOpen.value = true
    isLoadingSource.value = true
    sourceErrorMessage.value = ''
    sourceDetail.value = null
    activeSourceSubmissionId.value = submission.submission_id

    try {
      const response = await getSubmissionSource(submission.submission_id, authState.token)
      sourceDetail.value = {
        submission_id: Number(response.submission_id),
        language: response.language || submission.language,
        source_code: response.source_code || '',
        compile_output: typeof response.compile_output === 'string' ? response.compile_output : '',
        judge_output: typeof response.judge_output === 'string' ? response.judge_output : ''
      }
    } catch (error) {
      sourceErrorMessage.value = error instanceof Error
        ? error.message
        : '소스 코드를 불러오지 못했습니다.'
    } finally {
      isLoadingSource.value = false
    }
  }

  async function copySourceCode(){
    if (!sourceDetail.value?.source_code) {
      return
    }

    try {
      if (typeof navigator !== 'undefined' && navigator.clipboard?.writeText) {
        await navigator.clipboard.writeText(sourceDetail.value.source_code)
      } else {
        fallbackCopyText(sourceDetail.value.source_code)
      }

      copyState.value = 'success'
    } catch {
      copyState.value = 'error'
    }

    scheduleCopyStateReset()
  }

  onUnmounted(() => {
    resetCopyState()
  })

  return {
    historyDialogOpen,
    isLoadingHistory,
    historyErrorMessage,
    submissionHistoryEntries,
    activeHistorySubmissionId,
    sourceDialogOpen,
    isLoadingSource,
    sourceErrorMessage,
    sourceDetail,
    activeSourceSubmissionId,
    copyButtonLabel,
    latestHistoryStatus,
    canViewSource,
    formatHistoryTransition,
    closeHistoryDialog,
    fetchSubmissionHistory,
    openHistoryDialog,
    closeSourceDialog,
    openSourceDialog,
    copySourceCode
  }
}
