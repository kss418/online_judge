import { computed } from 'vue'

import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import { useTestcaseZipUploadAction } from '@/composables/adminShared/useTestcaseZipUploadAction'

export function useProblemTestcaseZipActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  testcaseZipFile,
  resetTestcaseZipSelection,
  applySelectedProblemVersion,
  loadSelectedProblem,
  setActionFeedback
}){
  const selectedProblemId = computed(() => Number(selectedProblemDetail.value?.problem_id ?? 0))
  const clearActionFeedback = () => setActionFeedback({
    message: '',
    error: ''
  })

  return useTestcaseZipUploadAction({
    authState,
    busySection,
    uploadSection: problemBusySection.UPLOAD_TESTCASE_ZIP,
    selectedProblemId,
    testcaseZipFile,
    clearFeedback: clearActionFeedback,
    resetTestcaseZipSelection,
    async afterUpload(response, problemId){
      applySelectedProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId, {
        skipTestcaseZipReset: true
      })
    },
    showSuccess(message){
      setActionFeedback({
        message
      })
    },
    showError(error){
      setActionFeedback({
        error
      })
    },
    formatSuccessMessage(response){
      const testcaseCount = Number(response.testcase_count ?? 0)
      return `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    },
    fallbackError: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
  })
}
