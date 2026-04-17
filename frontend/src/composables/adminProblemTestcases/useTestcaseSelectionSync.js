import { computed, ref } from 'vue'

import { parsePositiveInteger } from '@/utils/parse'

export function useTestcaseSelectionSync({
  authState,
  pageAccess,
  selectedProblemId,
  testcaseListResource,
  selectedTestcaseResource,
  viewTestcaseOrderInput,
  showErrorNotice
}){
  const selectedTestcaseOrder = ref(0)
  const selectedTestcaseSummary = computed(() =>
    testcaseListResource.testcaseItems.value.find(
      (testcase) => testcase.testcase_order === selectedTestcaseOrder.value
    ) || null
  )
  const canViewSpecificTestcase = computed(() => {
    if (!testcaseListResource.testcaseItems.value.length) {
      return false
    }

    return parsePositiveInteger(viewTestcaseOrderInput.value) != null
  })

  function resetSelectedTestcaseState(){
    selectedTestcaseOrder.value = 0
    viewTestcaseOrderInput.value = ''
    selectedTestcaseResource.clearSelectedTestcaseDetail()
  }

  function syncSelectedTestcase(preferredOrder){
    const testcaseItems = testcaseListResource.testcaseItems.value
    if (!testcaseItems.length) {
      resetSelectedTestcaseState()
      return
    }

    const lastTestcaseOrder = testcaseItems[testcaseItems.length - 1].testcase_order
    const candidateOrder =
      preferredOrder && preferredOrder > 0
        ? Math.min(preferredOrder, lastTestcaseOrder)
        : selectedTestcaseOrder.value > 0
          ? Math.min(selectedTestcaseOrder.value, lastTestcaseOrder)
          : testcaseItems[0].testcase_order
    const matchedTestcase = testcaseItems.find(
      (testcase) => testcase.testcase_order === candidateOrder
    )
    const nextOrder = matchedTestcase
      ? matchedTestcase.testcase_order
      : testcaseItems[0].testcase_order

    selectedTestcaseOrder.value = nextOrder
    viewTestcaseOrderInput.value = String(nextOrder)
  }

  function syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder){
    if (preferredTestcaseId > 0) {
      const matchedTestcase = testcaseListResource.testcaseItems.value.find(
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
    const normalizedTestcaseOrder = parsePositiveInteger(testcaseOrder)
    if (normalizedTestcaseOrder == null) {
      return
    }

    selectedTestcaseOrder.value = normalizedTestcaseOrder
    viewTestcaseOrderInput.value = String(normalizedTestcaseOrder)
  }

  function handleViewSelectedTestcase(){
    const targetOrder = parsePositiveInteger(viewTestcaseOrderInput.value)
    if (targetOrder == null) {
      showErrorNotice('확인할 테스트케이스 순번을 올바르게 입력하세요.')
      return false
    }

    const matchedTestcase = testcaseListResource.testcaseItems.value.find(
      (testcase) => testcase.testcase_order === targetOrder
    )
    if (!matchedTestcase) {
      showErrorNotice(`테스트케이스 ${targetOrder}번이 없습니다.`)
      return false
    }

    selectTestcase(matchedTestcase.testcase_order)
    return true
  }

  async function loadTestcases(preferredOrder){
    const result = await testcaseListResource.loadTestcases()

    if (result.status !== 'success') {
      resetSelectedTestcaseState()
      return result
    }

    syncSelectedTestcase(preferredOrder)
    return result
  }

  pageAccess.watchWhenAllowed(selectedTestcaseSummary, (testcaseSummary) => {
    if (!authState.token || selectedProblemId.value <= 0 || !testcaseSummary) {
      selectedTestcaseResource.clearSelectedTestcaseDetail()
      return
    }

    void selectedTestcaseResource.loadSelectedTestcaseDetail(testcaseSummary.testcase_order)
  })

  return {
    selectedTestcaseOrder,
    selectedTestcaseSummary,
    canViewSpecificTestcase,
    resetSelectedTestcaseState,
    syncSelectedTestcase,
    syncSelectedTestcaseById,
    selectTestcase,
    handleViewSelectedTestcase,
    loadTestcases
  }
}
