import { computed, onUnmounted, ref } from 'vue'

import {
  getSubmissionHistory,
  getSubmissionSource
} from '@/api/submission'
import { useAsyncResource } from '@/composables/useAsyncResource'
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
  const activeHistorySubmissionId = ref(null)
  const sourceDialogOpen = ref(false)
  const activeSourceSubmissionId = ref(null)
  const copyState = ref('idle')
  const historyResource = useAsyncResource({
    initialData: [],
    async load(submissionId){
      const response = await getSubmissionHistory(submissionId, authState.token)

      return Array.isArray(response.histories)
        ? response.histories.map((historyEntry, index) =>
          normalizeSubmissionHistoryEntry(historyEntry, index)
        )
        : []
    },
    getErrorMessage(error){
      return error instanceof Error
        ? error.message
        : '채점 내역을 불러오지 못했습니다.'
    }
  })
  const sourceResource = useAsyncResource({
    initialData: null,
    async load(submission){
      const response = await getSubmissionSource(submission.submission_id, authState.token)

      return {
        submission_id: Number(response.submission_id),
        language: response.language || submission.language,
        source_code: response.source_code || '',
        compile_output: typeof response.compile_output === 'string' ? response.compile_output : '',
        judge_output: typeof response.judge_output === 'string' ? response.judge_output : ''
      }
    },
    getErrorMessage(error){
      return error instanceof Error
        ? error.message
        : '소스 코드를 불러오지 못했습니다.'
    }
  })

  const isLoadingHistory = historyResource.isLoading
  const historyErrorMessage = historyResource.errorMessage
  const submissionHistoryEntries = historyResource.data
  const isLoadingSource = sourceResource.isLoading
  const sourceErrorMessage = sourceResource.errorMessage
  const sourceDetail = sourceResource.data
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
    historyDialogOpen.value = false
    activeHistorySubmissionId.value = null
    historyResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function fetchSubmissionHistory(submissionId, options = {}){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    const { background = false } = options
    const numericSubmissionId = Number(submissionId)

    if (!Number.isInteger(numericSubmissionId) || numericSubmissionId <= 0) {
      return
    }

    const result = await historyResource.run(numericSubmissionId, {
      background,
      clearErrorOnRun: !background,
      resetDataOnRun: !background,
      resetDataOnError: !background,
      setErrorOnError: !background
    })

    if (result.status !== 'success') {
      return
    }

    const latestHistoryEntry = result.data[result.data.length - 1]
    if (latestHistoryEntry) {
      patchSubmission(numericSubmissionId, {
        status: latestHistoryEntry.to_status
      })
    }
  }

  async function openHistoryDialog(submission){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    historyDialogOpen.value = true
    activeHistorySubmissionId.value = submission.submission_id

    await fetchSubmissionHistory(submission.submission_id)
  }

  function closeSourceDialog(){
    sourceDialogOpen.value = false
    activeSourceSubmissionId.value = null
    sourceResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
    resetCopyState()
  }

  async function openSourceDialog(submission){
    if (!canViewSource(submission) || !authState.token) {
      return
    }

    sourceDialogOpen.value = true
    activeSourceSubmissionId.value = submission.submission_id

    await sourceResource.run(submission, {
      resetDataOnRun: true,
      resetDataOnError: true
    })
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
