import { computed, nextTick, ref, watch } from 'vue'

import { parsePositiveInteger } from '@/utils/parse'

export function useTestcaseSelectionState({
  authState,
  canAccessPage,
  selectedProblemId,
  testcaseListResource,
  selectedTestcaseResource,
  showErrorNotice
}){
  const testcaseSummaryElementMap = new Map()
  const selectedTestcaseOrder = ref(0)
  const viewTestcaseOrderInput = ref('')
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
    testcaseSummaryElementMap.clear()
    selectedTestcaseResource.clearSelectedTestcaseDetail()
  }

  async function scrollSelectedTestcaseIntoView(){
    await nextTick()

    const summaryElement = testcaseSummaryElementMap.get(selectedTestcaseOrder.value)
    if (!summaryElement || typeof summaryElement.scrollIntoView !== 'function') {
      return
    }

    summaryElement.scrollIntoView({
      block: 'nearest',
      inline: 'nearest'
    })
  }

  function syncSelectedTestcase(preferredOrder, options = {}){
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

    if (options.scroll === true) {
      void scrollSelectedTestcaseIntoView()
    }
  }

  function syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder, options = {}){
    if (preferredTestcaseId > 0) {
      const matchedTestcase = testcaseListResource.testcaseItems.value.find(
        (testcase) => testcase.testcase_id === preferredTestcaseId
      )

      if (matchedTestcase) {
        syncSelectedTestcase(matchedTestcase.testcase_order, options)
        return
      }
    }

    syncSelectedTestcase(fallbackOrder, options)
  }

  function selectTestcase(testcaseOrder){
    const normalizedTestcaseOrder = parsePositiveInteger(testcaseOrder)
    if (normalizedTestcaseOrder == null) {
      return
    }

    selectedTestcaseOrder.value = normalizedTestcaseOrder
    viewTestcaseOrderInput.value = String(normalizedTestcaseOrder)
    void scrollSelectedTestcaseIntoView()
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

  async function loadTestcases(preferredOrder, options = {}){
    const result = await testcaseListResource.loadTestcases()

    if (result.status !== 'success') {
      resetSelectedTestcaseState()
      return result
    }

    syncSelectedTestcase(preferredOrder, options)
    return result
  }

  function setTestcaseSummaryElement(testcaseOrder, element){
    if (!element) {
      testcaseSummaryElementMap.delete(testcaseOrder)
      return
    }

    testcaseSummaryElementMap.set(testcaseOrder, element)
  }

  watch([selectedTestcaseSummary, canAccessPage], ([testcaseSummary, isAllowed]) => {
    if (!isAllowed || !authState.token || selectedProblemId.value <= 0 || !testcaseSummary) {
      selectedTestcaseResource.clearSelectedTestcaseDetail()
      return
    }

    void selectedTestcaseResource.loadSelectedTestcaseDetail(testcaseSummary.testcase_order)
  }, {
    immediate: true
  })

  return {
    viewTestcaseOrderInput,
    selectedTestcaseOrder,
    selectedTestcaseSummary,
    canViewSpecificTestcase,
    resetSelectedTestcaseState,
    syncSelectedTestcase,
    syncSelectedTestcaseById,
    selectTestcase,
    handleViewSelectedTestcase,
    loadTestcases,
    setTestcaseSummaryElement
  }
}
