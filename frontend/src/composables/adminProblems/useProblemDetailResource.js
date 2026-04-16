import { getProblemDetail } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useProblemDetailResource({
  authState,
  selectedProblemId
}){
  const detailResource = useAsyncResource({
    initialData: null,
    async load(problemId){
      return getProblemDetail(problemId, {
        bearerToken: authState.token || ''
      })
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 정보를 불러오지 못했습니다.'
      })
    }
  })

  const isLoadingDetail = detailResource.isLoading
  const detailErrorMessage = detailResource.errorMessage
  const selectedProblemDetail = detailResource.data

  function invalidateSelectedProblemResource(){
    detailResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  function setSelectedProblemDetail(nextValueOrUpdater){
    detailResource.mutate(nextValueOrUpdater)
  }

  function setSelectedProblemSamples(samples){
    if (!selectedProblemDetail.value) {
      return
    }

    setSelectedProblemDetail((problemDetail) => {
      if (!problemDetail) {
        return problemDetail
      }

      return {
        ...problemDetail,
        samples: Array.isArray(samples) ? samples : []
      }
    })
  }

  function applySelectedProblemVersion(problemId, version){
    const normalizedVersion = Number(version)
    if (
      !Number.isInteger(normalizedVersion) ||
      normalizedVersion <= 0 ||
      selectedProblemDetail.value?.problem_id !== problemId
    ) {
      return
    }

    setSelectedProblemDetail((problemDetail) => {
      if (!problemDetail) {
        return problemDetail
      }

      return {
        ...problemDetail,
        version: normalizedVersion
      }
    })
  }

  async function loadProblemDetail(problemId = selectedProblemId.value){
    if (!problemId) {
      invalidateSelectedProblemResource()
      return {
        status: 'reset'
      }
    }

    return detailResource.run(problemId, {
      resetDataOnRun: true,
      resetDataOnError: true
    })
  }

  return {
    isLoadingDetail,
    detailErrorMessage,
    selectedProblemDetail,
    invalidateSelectedProblemResource,
    setSelectedProblemDetail,
    setSelectedProblemSamples,
    applySelectedProblemVersion,
    loadProblemDetail
  }
}
