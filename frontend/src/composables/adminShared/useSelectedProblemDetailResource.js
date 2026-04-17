import { getProblemDetail } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

function normalizeProblemId(problemId){
  const normalizedProblemId = Number(problemId)
  return Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
    ? normalizedProblemId
    : 0
}

function normalizeVersion(version){
  const normalizedVersion = Number(version)
  return Number.isInteger(normalizedVersion) && normalizedVersion > 0
    ? normalizedVersion
    : 0
}

export function useSelectedProblemDetailResource({
  authState,
  selectedProblemId,
  mergeProblemSummary
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

  function syncProblemSummary(problemId, patch){
    if (typeof mergeProblemSummary !== 'function') {
      return
    }

    const normalizedProblemId = normalizeProblemId(problemId)
    if (!normalizedProblemId) {
      return
    }

    mergeProblemSummary(normalizedProblemId, patch)
  }

  function resetSelectedProblemDetail(){
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

  function applyProblemVersion(problemId, version){
    const normalizedProblemId = normalizeProblemId(problemId)
    const normalizedVersion = normalizeVersion(version)

    if (!normalizedProblemId || !normalizedVersion) {
      return
    }

    if (selectedProblemDetail.value?.problem_id === normalizedProblemId) {
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

    syncProblemSummary(normalizedProblemId, {
      version: normalizedVersion
    })
  }

  async function loadProblemDetail(problemId = selectedProblemId.value){
    const normalizedProblemId = normalizeProblemId(problemId)
    if (!normalizedProblemId) {
      resetSelectedProblemDetail()
      return {
        status: 'reset'
      }
    }

    const result = await detailResource.run(normalizedProblemId, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return result
    }

    syncProblemSummary(normalizedProblemId, {
      title: result.data.title,
      version: result.data.version
    })

    return result
  }

  return {
    isLoadingDetail,
    detailErrorMessage,
    selectedProblemDetail,
    resetSelectedProblemDetail,
    setSelectedProblemDetail,
    setSelectedProblemSamples,
    applyProblemVersion,
    loadProblemDetail
  }
}
