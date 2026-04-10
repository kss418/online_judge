import { computed } from 'vue'

import { moveProblemTestcase } from '@/api/problem'

export function useTestcaseReorder({
  authState,
  busySection,
  testcaseItems,
  selectedProblemId,
  selectedTestcaseSummary,
  applyProblemVersion,
  showErrorNotice,
  showSuccessNotice,
  syncSelectedTestcaseById
}){
  const isMovingTestcase = computed(() => busySection.value === 'move')
  const canMoveTestcases = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    testcaseItems.value.length > 1 &&
    !busySection.value
  )

  function reorderTestcaseItems(sourceTestcaseOrder, targetTestcaseOrder){
    const nextTestcaseItems = testcaseItems.value.map((testcase) => {
      if (testcase.testcase_order === sourceTestcaseOrder) {
        return {
          ...testcase,
          testcase_order: targetTestcaseOrder
        }
      }

      if (
        sourceTestcaseOrder < targetTestcaseOrder &&
        testcase.testcase_order > sourceTestcaseOrder &&
        testcase.testcase_order <= targetTestcaseOrder
      ) {
        return {
          ...testcase,
          testcase_order: testcase.testcase_order - 1
        }
      }

      if (
        sourceTestcaseOrder > targetTestcaseOrder &&
        testcase.testcase_order >= targetTestcaseOrder &&
        testcase.testcase_order < sourceTestcaseOrder
      ) {
        return {
          ...testcase,
          testcase_order: testcase.testcase_order + 1
        }
      }

      return testcase
    })

    nextTestcaseItems.sort((left, right) => left.testcase_order - right.testcase_order)
    testcaseItems.value = nextTestcaseItems
  }

  async function handleMoveTestcase({ sourceTestcaseOrder, targetTestcaseOrder }){
    if (!canMoveTestcases.value || !authState.token) {
      return
    }

    const normalizedSourceOrder = Number(sourceTestcaseOrder)
    const normalizedTargetOrder = Number(targetTestcaseOrder)

    if (
      !Number.isInteger(normalizedSourceOrder) ||
      !Number.isInteger(normalizedTargetOrder) ||
      normalizedSourceOrder <= 0 ||
      normalizedTargetOrder <= 0 ||
      normalizedSourceOrder === normalizedTargetOrder
    ) {
      return
    }

    const selectedTestcaseId = Number(selectedTestcaseSummary.value?.testcase_id ?? 0)
    busySection.value = 'move'

    try {
      const response = await moveProblemTestcase(
        selectedProblemId.value,
        {
          source_testcase_order: normalizedSourceOrder,
          target_testcase_order: normalizedTargetOrder
        },
        authState.token
      )

      applyProblemVersion(selectedProblemId.value, response.version)
      reorderTestcaseItems(normalizedSourceOrder, normalizedTargetOrder)
      syncSelectedTestcaseById(selectedTestcaseId, normalizedTargetOrder)
      showSuccessNotice(
        `테스트케이스 ${normalizedSourceOrder}번을 ${normalizedTargetOrder}번으로 이동했습니다.`
      )
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스 순서를 변경하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  return {
    isMovingTestcase,
    canMoveTestcases,
    handleMoveTestcase
  }
}
