import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import { uploadProblemTestcaseZip } from '@/api/testcaseApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemTestcaseZipActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  testcaseZipFile,
  applySelectedProblemVersion,
  loadSelectedProblem,
  setActionFeedback
}){
  const isUploadingTestcaseZip = computed(() => busySection.value === problemBusySection.UPLOAD_TESTCASE_ZIP)
  const canUploadTestcaseZip = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )
  const clearActionFeedback = () => setActionFeedback({
    message: '',
    error: ''
  })

  async function handleUploadTestcaseZip(){
    if (!authState.token || !selectedProblemDetail.value || !canUploadTestcaseZip.value || !testcaseZipFile.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const uploadFile = testcaseZipFile.value

    return runBusyAction({
      busySection,
      section: problemBusySection.UPLOAD_TESTCASE_ZIP,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await uploadProblemTestcaseZip(problemId, uploadFile, authState.token)
        applySelectedProblemVersion(problemId, response.version)
        await loadSelectedProblem(problemId)

        const testcaseCount = Number(response.testcase_count ?? 0)
        setActionFeedback({
          message: `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
          error: formatApiError(error, {
            fallback: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
          })
        })
      }
    })
  }

  return {
    isUploadingTestcaseZip,
    canUploadTestcaseZip,
    handleUploadTestcaseZip
  }
}
