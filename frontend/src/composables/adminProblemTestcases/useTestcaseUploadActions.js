import { computed } from 'vue'

import {
  createProblemTestcase,
  deleteProblemTestcase,
  updateProblemTestcase,
  uploadProblemTestcaseZip
} from '@/api/problem'

export function useTestcaseUploadActions({
  authState,
  busySection,
  formatCount,
  showErrorNotice,
  showSuccessNotice,
  selectedProblemId,
  testcaseItems,
  selectedTestcase,
  selectedTestcaseSummary,
  newTestcaseInput,
  newTestcaseOutput,
  testcaseZipFile,
  testcaseZipInputKey,
  selectedTestcaseInputDraft,
  selectedTestcaseOutputDraft,
  canSaveSelectedTestcase,
  applyProblemVersion,
  loadProblems,
  loadSelectedProblemData,
  loadTestcases,
  syncSelectedTestcase,
  resetSelectedTestcaseState
}){
  const isCreatingTestcase = computed(() => busySection.value === 'create')
  const isUploadingTestcaseZip = computed(() => busySection.value === 'upload')
  const isDeletingSelectedTestcase = computed(() => busySection.value === 'delete-selected')
  const isSavingSelectedTestcase = computed(() => busySection.value === 'save')
  const canCreateTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canUploadTestcaseZip = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )
  const canDeleteSelectedTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    Boolean(selectedTestcaseSummary.value) &&
    !busySection.value
  )

  async function handleCreateTestcase(){
    if (!canCreateTestcase.value || !authState.token) {
      return
    }

    busySection.value = 'create'

    const nextTestcaseInput = newTestcaseInput.value
    const nextTestcaseOutput = newTestcaseOutput.value

    try {
      const response = await createProblemTestcase(
        selectedProblemId.value,
        {
          testcase_input: nextTestcaseInput,
          testcase_output: nextTestcaseOutput
        },
        authState.token
      )

      applyProblemVersion(selectedProblemId.value, response.version)
      await loadTestcases(Number(response.testcase_order ?? 0))
      newTestcaseInput.value = ''
      newTestcaseOutput.value = ''
      showSuccessNotice('테스트케이스를 마지막에 추가했습니다.')
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 추가하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleUploadTestcaseZip(){
    if (!canUploadTestcaseZip.value || !authState.token || !testcaseZipFile.value) {
      return
    }

    busySection.value = 'upload'
    const uploadFile = testcaseZipFile.value

    try {
      const response = await uploadProblemTestcaseZip(selectedProblemId.value, uploadFile, authState.token)
      testcaseZipFile.value = null
      testcaseZipInputKey.value += 1
      applyProblemVersion(selectedProblemId.value, response.version)
      await Promise.all([
        loadProblems(),
        loadSelectedProblemData()
      ])

      const uploadedTestcaseCount = Number(response.testcase_count ?? 0)
      showSuccessNotice(`테스트케이스 ${formatCount(uploadedTestcaseCount)}개를 업로드했습니다.`)
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스 ZIP을 업로드하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteSelectedTestcase(){
    if (!canDeleteSelectedTestcase.value || !authState.token || !selectedTestcaseSummary.value) {
      return
    }

    const deletedTestcaseOrder = selectedTestcaseSummary.value.testcase_order

    busySection.value = 'delete-selected'

    try {
      const response = await deleteProblemTestcase(
        selectedProblemId.value,
        deletedTestcaseOrder,
        authState.token
      )
      applyProblemVersion(selectedProblemId.value, response.version)
      testcaseItems.value = testcaseItems.value
        .filter((testcase) => testcase.testcase_order !== deletedTestcaseOrder)
        .map((testcase) => ({
          ...testcase,
          testcase_order: testcase.testcase_order > deletedTestcaseOrder
            ? testcase.testcase_order - 1
            : testcase.testcase_order
        }))
      syncSelectedTestcase(deletedTestcaseOrder)
      showSuccessNotice(`테스트케이스 ${deletedTestcaseOrder}번을 삭제했습니다.`)
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 삭제하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveSelectedTestcase(){
    if (!selectedTestcase.value || !canSaveSelectedTestcase.value || !authState.token) {
      return
    }

    busySection.value = 'save'

    const testcaseOrder = selectedTestcase.value.testcase_order
    const nextTestcaseInput = selectedTestcaseInputDraft.value
    const nextTestcaseOutput = selectedTestcaseOutputDraft.value

    try {
      const response = await updateProblemTestcase(
        selectedProblemId.value,
        testcaseOrder,
        {
          testcase_input: nextTestcaseInput,
          testcase_output: nextTestcaseOutput
        },
        authState.token
      )

      applyProblemVersion(selectedProblemId.value, response.version)
      selectedTestcase.value = {
        testcase_id: Number(response?.testcase_id ?? 0),
        testcase_order: Number(response?.testcase_order ?? 0),
        testcase_input: typeof response?.testcase_input === 'string' ? response.testcase_input : '',
        testcase_output: typeof response?.testcase_output === 'string' ? response.testcase_output : ''
      }
      await loadTestcases(testcaseOrder)
      showSuccessNotice(`테스트케이스 ${testcaseOrder}번을 저장했습니다.`)
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 저장하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  return {
    isCreatingTestcase,
    isUploadingTestcaseZip,
    isDeletingSelectedTestcase,
    isSavingSelectedTestcase,
    canCreateTestcase,
    canUploadTestcaseZip,
    canDeleteSelectedTestcase,
    handleCreateTestcase,
    handleUploadTestcaseZip,
    handleDeleteSelectedTestcase,
    handleSaveSelectedTestcase
  }
}
