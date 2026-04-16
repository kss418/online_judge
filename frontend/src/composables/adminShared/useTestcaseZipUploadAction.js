import { computed } from 'vue'

import { uploadProblemTestcaseZip } from '@/api/testcaseApi'
import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { formatApiError } from '@/utils/apiError'

export function useTestcaseZipUploadAction({
  authState,
  busySection,
  uploadSection,
  selectedProblemId,
  testcaseZipFile,
  clearFeedback,
  resetTestcaseZipSelection,
  afterUpload,
  showSuccess,
  showError,
  formatSuccessMessage,
  fallbackError
}){
  const isUploadingTestcaseZip = computed(() => busySection.value === uploadSection)
  const canUploadTestcaseZip = computed(() =>
    Number(selectedProblemId.value) > 0 &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )

  async function handleUploadTestcaseZip(){
    if (!canUploadTestcaseZip.value || !authState.token || !testcaseZipFile.value) {
      return
    }

    const problemId = Number(selectedProblemId.value)
    const uploadFile = testcaseZipFile.value

    return runBusyAction({
      busySection,
      section: uploadSection,
      clearFeedback,
      run: async () => {
        const response = await uploadProblemTestcaseZip(problemId, uploadFile, authState.token)
        resetTestcaseZipSelection?.()
        await afterUpload?.(response, problemId)
        showSuccess?.(formatSuccessMessage(response))
        return response
      },
      onError: (error) => {
        showError?.(formatApiError(error, {
          fallback: fallbackError
        }))
      }
    })
  }

  return {
    isUploadingTestcaseZip,
    canUploadTestcaseZip,
    handleUploadTestcaseZip
  }
}
