import { computed, ref } from 'vue'

import { useTestcaseZipInput } from '@/composables/adminShared/useTestcaseZipInput'
import { parsePositiveInteger } from '@/utils/parse'

export function useTestcaseEditorDraft({
  authState,
  busySection,
  testcaseItems,
  selectedTestcase,
  showErrorNotice
}){
  const newTestcaseInput = ref('')
  const newTestcaseOutput = ref('')
  const selectedTestcaseOrder = ref(0)
  const selectedTestcaseInputDraft = ref('')
  const selectedTestcaseOutputDraft = ref('')
  const viewTestcaseOrderInput = ref('')
  const {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  } = useTestcaseZipInput({
    onInvalidZip(){
      showErrorNotice('ZIP 파일만 업로드할 수 있습니다.')
    }
  })

  const selectedTestcaseSummary = computed(() =>
    testcaseItems.value.find((testcase) => testcase.testcase_order === selectedTestcaseOrder.value) || null
  )
  const canViewSpecificTestcase = computed(() => {
    if (!testcaseItems.value.length) {
      return false
    }

    return parsePositiveInteger(viewTestcaseOrderInput.value) != null
  })
  const canSaveSelectedTestcase = computed(() => {
    if (!selectedTestcase.value || !authState.token || busySection.value) {
      return false
    }

    return (
      selectedTestcaseInputDraft.value !== selectedTestcase.value.testcase_input ||
      selectedTestcaseOutputDraft.value !== selectedTestcase.value.testcase_output
    )
  })

  function resetDraftState(){
    newTestcaseInput.value = ''
    newTestcaseOutput.value = ''
    resetTestcaseZipSelection()
    selectedTestcaseOrder.value = 0
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    viewTestcaseOrderInput.value = ''
  }

  function resetSelectedTestcaseState(){
    selectedTestcaseOrder.value = 0
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    viewTestcaseOrderInput.value = ''
  }

  function syncSelectedTestcase(preferredOrder){
    if (!testcaseItems.value.length) {
      resetSelectedTestcaseState()
      return
    }

    const lastTestcaseOrder = testcaseItems.value[testcaseItems.value.length - 1].testcase_order
    const candidateOrder =
      preferredOrder && preferredOrder > 0
        ? Math.min(preferredOrder, lastTestcaseOrder)
        : selectedTestcaseOrder.value > 0
          ? Math.min(selectedTestcaseOrder.value, lastTestcaseOrder)
          : testcaseItems.value[0].testcase_order
    const matchedTestcase = testcaseItems.value.find(
      (testcase) => testcase.testcase_order === candidateOrder
    )
    const nextOrder = matchedTestcase
      ? matchedTestcase.testcase_order
      : testcaseItems.value[0].testcase_order

    selectedTestcaseOrder.value = nextOrder
    viewTestcaseOrderInput.value = String(nextOrder)
  }

  function syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder){
    if (preferredTestcaseId > 0) {
      const matchedTestcase = testcaseItems.value.find(
        (testcase) => testcase.testcase_id === preferredTestcaseId
      )

      if (matchedTestcase) {
        syncSelectedTestcase(matchedTestcase.testcase_order)
        return
      }
    }

    syncSelectedTestcase(fallbackOrder)
  }

  function selectTestcase(testcaseOrder){
    selectedTestcaseOrder.value = testcaseOrder
    viewTestcaseOrderInput.value = String(testcaseOrder)
  }

  function handleViewSelectedTestcase(){
    const targetOrder = parsePositiveInteger(viewTestcaseOrderInput.value)
    if (targetOrder == null) {
      showErrorNotice('확인할 테스트케이스 순번을 올바르게 입력하세요.')
      return false
    }

    const matchedTestcase = testcaseItems.value.find(
      (testcase) => testcase.testcase_order === targetOrder
    )
    if (!matchedTestcase) {
      showErrorNotice(`테스트케이스 ${targetOrder}번이 없습니다.`)
      return false
    }

    selectTestcase(matchedTestcase.testcase_order)
    return true
  }

  return {
    newTestcaseInput,
    newTestcaseOutput,
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseOrder,
    selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft,
    viewTestcaseOrderInput,
    selectedTestcaseZipName,
    selectedTestcaseSummary,
    canViewSpecificTestcase,
    canSaveSelectedTestcase,
    resetDraftState,
    resetTestcaseZipSelection,
    resetSelectedTestcaseState,
    syncSelectedTestcase,
    syncSelectedTestcaseById,
    selectTestcase,
    handleViewSelectedTestcase,
    handleTestcaseZipFileChange
  }
}
