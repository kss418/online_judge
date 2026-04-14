import { computed } from 'vue'

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
  const isUploadingTestcaseZip = computed(() => busySection.value === 'testcase:upload')
  const canUploadTestcaseZip = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )

  async function handleUploadTestcaseZip(){
    if (!authState.token || !selectedProblemDetail.value || !canUploadTestcaseZip.value || !testcaseZipFile.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const uploadFile = testcaseZipFile.value
    busySection.value = 'testcase:upload'
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      const response = await uploadProblemTestcaseZip(problemId, uploadFile, authState.token)
      applySelectedProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId)

      const testcaseCount = Number(response.testcase_count ?? 0)
      setActionFeedback({
        message: `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
      })
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  return {
    isUploadingTestcaseZip,
    canUploadTestcaseZip,
    handleUploadTestcaseZip
  }
}
