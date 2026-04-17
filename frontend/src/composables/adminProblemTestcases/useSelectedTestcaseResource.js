import { getProblemTestcase } from '@/api/testcaseApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

function normalizeTestcaseOrder(testcaseOrder){
  const normalizedTestcaseOrder = Number(testcaseOrder)
  return Number.isInteger(normalizedTestcaseOrder) && normalizedTestcaseOrder > 0
    ? normalizedTestcaseOrder
    : 0
}

export function useSelectedTestcaseResource({
  authState,
  selectedProblemId
}){
  const selectedTestcaseResource = useAsyncResource({
    initialData: null,
    async load({ problemId, testcaseOrder }){
      return getProblemTestcase(problemId, testcaseOrder, {
        bearerToken: authState.token || ''
      })
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '테스트케이스 본문을 불러오지 못했습니다.'
      })
    }
  })

  function clearSelectedTestcaseDetail(){
    selectedTestcaseResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  function setSelectedTestcase(nextValueOrUpdater){
    selectedTestcaseResource.mutate(nextValueOrUpdater)
  }

  async function loadSelectedTestcaseDetail(testcaseOrder){
    const normalizedProblemId = Number(selectedProblemId.value)
    const normalizedTestcaseOrder = normalizeTestcaseOrder(testcaseOrder)

    if (
      !Number.isInteger(normalizedProblemId) ||
      normalizedProblemId <= 0 ||
      !normalizedTestcaseOrder
    ) {
      clearSelectedTestcaseDetail()
      return {
        status: 'reset'
      }
    }

    return selectedTestcaseResource.run({
      problemId: normalizedProblemId,
      testcaseOrder: normalizedTestcaseOrder
    }, {
      resetDataOnRun: true,
      resetDataOnError: true
    })
  }

  return {
    selectedTestcase: selectedTestcaseResource.data,
    isLoadingSelectedTestcase: selectedTestcaseResource.isLoading,
    selectedTestcaseErrorMessage: selectedTestcaseResource.errorMessage,
    clearSelectedTestcaseDetail,
    setSelectedTestcase,
    loadSelectedTestcaseDetail
  }
}
