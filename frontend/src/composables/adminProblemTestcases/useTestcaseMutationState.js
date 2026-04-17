import { computed } from 'vue'

import {
  createProblemTestcase,
  deleteProblemTestcase,
  moveProblemTestcase,
  updateProblemTestcase
} from '@/api/testcaseApi'
import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { testcaseBusySection } from '@/composables/adminProblemTestcases/testcaseBusySection'
import { formatApiError } from '@/utils/apiError'

function normalizeSelectedTestcaseDetail(response){
  return {
    testcase_id: Number(response?.testcase_id ?? 0),
    testcase_order: Number(response?.testcase_order ?? 0),
    testcase_input: typeof response?.testcase_input === 'string' ? response.testcase_input : '',
    testcase_output: typeof response?.testcase_output === 'string' ? response.testcase_output : ''
  }
}

function reorderTestcaseItems(testcaseItems, sourceTestcaseOrder, targetTestcaseOrder){
  const nextTestcaseItems = testcaseItems.map((testcase) => {
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
  return nextTestcaseItems
}

export function useTestcaseMutationState({
  authState,
  busySection,
  selectedProblemId,
  draftState,
  selectionState,
  testcaseListResource,
  selectedTestcaseResource,
  problemDetailResource,
  showErrorNotice,
  showSuccessNotice
}){
  const isCreatingTestcase = computed(() => busySection.value === testcaseBusySection.CREATE)
  const isDeletingSelectedTestcase = computed(() => busySection.value === testcaseBusySection.DELETE_SELECTED)
  const isSavingSelectedTestcase = computed(() => busySection.value === testcaseBusySection.SAVE_SELECTED)
  const isMovingTestcase = computed(() => busySection.value === testcaseBusySection.MOVE)
  const canCreateTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canDeleteSelectedTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    Boolean(selectionState.selectedTestcaseSummary.value) &&
    !busySection.value
  )
  const canMoveTestcases = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    testcaseListResource.testcaseItems.value.length > 1 &&
    !busySection.value
  )

  async function handleCreateTestcase(){
    if (!canCreateTestcase.value || !authState.token) {
      return
    }

    const nextTestcaseInput = draftState.newTestcaseInput.value
    const nextTestcaseOutput = draftState.newTestcaseOutput.value

    return runBusyAction({
      busySection,
      section: testcaseBusySection.CREATE,
      run: async () => {
        const response = await createProblemTestcase(
          selectedProblemId.value,
          {
            testcase_input: nextTestcaseInput,
            testcase_output: nextTestcaseOutput
          },
          authState.token
        )

        problemDetailResource.applyProblemVersion(selectedProblemId.value, response.version)
        await selectionState.loadTestcases(Number(response.testcase_order ?? 0))
        draftState.newTestcaseInput.value = ''
        draftState.newTestcaseOutput.value = ''
        showSuccessNotice('테스트케이스를 마지막에 추가했습니다.')
        return response
      },
      onError: (error) => {
        showErrorNotice(
          formatApiError(error, {
            fallback: '테스트케이스를 추가하지 못했습니다.'
          })
        )
      }
    })
  }

  async function handleDeleteSelectedTestcase(){
    if (!canDeleteSelectedTestcase.value || !authState.token || !selectionState.selectedTestcaseSummary.value) {
      return
    }

    const deletedTestcaseOrder = selectionState.selectedTestcaseSummary.value.testcase_order

    return runBusyAction({
      busySection,
      section: testcaseBusySection.DELETE_SELECTED,
      run: async () => {
        const response = await deleteProblemTestcase(
          selectedProblemId.value,
          deletedTestcaseOrder,
          authState.token
        )

        problemDetailResource.applyProblemVersion(selectedProblemId.value, response.version)
        testcaseListResource.setTestcaseItems((testcaseItems) => testcaseItems
          .filter((testcase) => testcase.testcase_order !== deletedTestcaseOrder)
          .map((testcase) => ({
            ...testcase,
            testcase_order: testcase.testcase_order > deletedTestcaseOrder
              ? testcase.testcase_order - 1
              : testcase.testcase_order
          })))
        selectionState.syncSelectedTestcase(deletedTestcaseOrder)
        showSuccessNotice(`테스트케이스 ${deletedTestcaseOrder}번을 삭제했습니다.`)
        return response
      },
      onError: (error) => {
        showErrorNotice(
          formatApiError(error, {
            fallback: '테스트케이스를 삭제하지 못했습니다.'
          })
        )
      }
    })
  }

  async function handleSaveSelectedTestcase(){
    if (!selectedTestcaseResource.selectedTestcase.value || !draftState.canSaveSelectedTestcase.value || !authState.token) {
      return
    }

    const testcaseOrder = selectedTestcaseResource.selectedTestcase.value.testcase_order
    const nextTestcaseInput = draftState.selectedTestcaseInputDraft.value
    const nextTestcaseOutput = draftState.selectedTestcaseOutputDraft.value

    return runBusyAction({
      busySection,
      section: testcaseBusySection.SAVE_SELECTED,
      run: async () => {
        const response = await updateProblemTestcase(
          selectedProblemId.value,
          testcaseOrder,
          {
            testcase_input: nextTestcaseInput,
            testcase_output: nextTestcaseOutput
          },
          authState.token
        )

        problemDetailResource.applyProblemVersion(selectedProblemId.value, response.version)
        selectedTestcaseResource.setSelectedTestcase(normalizeSelectedTestcaseDetail(response))
        await selectionState.loadTestcases(testcaseOrder)
        showSuccessNotice(`테스트케이스 ${testcaseOrder}번을 저장했습니다.`)
        return response
      },
      onError: (error) => {
        showErrorNotice(
          formatApiError(error, {
            fallback: '테스트케이스를 저장하지 못했습니다.'
          })
        )
      }
    })
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

    const selectedTestcaseId = Number(selectionState.selectedTestcaseSummary.value?.testcase_id ?? 0)

    return runBusyAction({
      busySection,
      section: testcaseBusySection.MOVE,
      run: async () => {
        const response = await moveProblemTestcase(
          selectedProblemId.value,
          {
            source_testcase_order: normalizedSourceOrder,
            target_testcase_order: normalizedTargetOrder
          },
          authState.token
        )

        problemDetailResource.applyProblemVersion(selectedProblemId.value, response.version)
        testcaseListResource.setTestcaseItems((testcaseItems) =>
          reorderTestcaseItems(testcaseItems, normalizedSourceOrder, normalizedTargetOrder)
        )
        selectionState.syncSelectedTestcaseById(selectedTestcaseId, normalizedTargetOrder, {
          scroll: true
        })
        showSuccessNotice(
          `테스트케이스 ${normalizedSourceOrder}번을 ${normalizedTargetOrder}번으로 이동했습니다.`
        )
        return response
      },
      onError: (error) => {
        showErrorNotice(
          formatApiError(error, {
            fallback: '테스트케이스 순서를 변경하지 못했습니다.'
          })
        )
      }
    })
  }

  return {
    isCreatingTestcase,
    isDeletingSelectedTestcase,
    isSavingSelectedTestcase,
    isMovingTestcase,
    canCreateTestcase,
    canDeleteSelectedTestcase,
    canMoveTestcases,
    handleCreateTestcase,
    handleDeleteSelectedTestcase,
    handleSaveSelectedTestcase,
    handleMoveTestcase
  }
}
