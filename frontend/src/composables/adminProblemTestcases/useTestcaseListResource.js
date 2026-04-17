import { computed } from 'vue'

import { getProblemTestcases } from '@/api/testcaseApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useTestcaseListResource({
  authState,
  selectedProblemId
}){
  const testcaseListResource = useAsyncResource({
    initialData: [],
    async load(problemId){
      const response = await getProblemTestcases(problemId, {
        bearerToken: authState.token
      })

      return response.testcases
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '테스트케이스를 불러오지 못했습니다.'
      })
    }
  })

  const testcaseItems = testcaseListResource.data
  const testcaseCount = computed(() => testcaseItems.value.length)

  function resetTestcaseList(){
    testcaseListResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  function setTestcaseItems(nextValueOrUpdater){
    testcaseListResource.mutate(nextValueOrUpdater)
  }

  async function loadTestcases(){
    if (!authState.token || selectedProblemId.value <= 0) {
      resetTestcaseList()
      return {
        status: 'reset'
      }
    }

    return testcaseListResource.run(selectedProblemId.value, {
      resetDataOnError: true
    })
  }

  return {
    testcaseItems,
    testcaseCount,
    isLoadingTestcases: testcaseListResource.isLoading,
    testcaseErrorMessage: testcaseListResource.errorMessage,
    resetTestcaseList,
    setTestcaseItems,
    loadTestcases
  }
}
