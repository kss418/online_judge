import { useTestcaseZipUploadAction } from '@/composables/adminShared/useTestcaseZipUploadAction'
import { testcaseBusySection } from '@/composables/adminProblemTestcases/testcaseBusySection'

export function useTestcaseUploadState({
  authState,
  busySection,
  formatCount,
  selectedProblemId,
  testcaseZipFile,
  resetZipSelection,
  problemDetailResource,
  reloadProblems,
  reloadSelectedProblemData,
  showErrorNotice,
  showSuccessNotice
}){
  const testcaseZipUploadAction = useTestcaseZipUploadAction({
    authState,
    busySection,
    uploadSection: testcaseBusySection.UPLOAD_ZIP,
    selectedProblemId,
    testcaseZipFile,
    resetTestcaseZipSelection: resetZipSelection,
    async afterUpload(response, problemId){
      problemDetailResource.applyProblemVersion(problemId, response.version)
      await Promise.all([
        reloadProblems(),
        reloadSelectedProblemData()
      ])
    },
    showSuccess(message){
      showSuccessNotice(message)
    },
    showError(error){
      showErrorNotice(error)
    },
    formatSuccessMessage(response){
      const uploadedTestcaseCount = Number(response.testcase_count ?? 0)
      return `테스트케이스 ${formatCount(uploadedTestcaseCount)}개를 업로드했습니다.`
    },
    fallbackError: '테스트케이스 ZIP을 업로드하지 못했습니다.'
  })

  return {
    canUploadTestcaseZip: testcaseZipUploadAction.canUploadTestcaseZip,
    isUploadingTestcaseZip: testcaseZipUploadAction.isUploadingTestcaseZip,
    handleUploadTestcaseZip: testcaseZipUploadAction.handleUploadTestcaseZip
  }
}
