import { ref } from 'vue'

import { getProblemDetail } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useProblemDetailResource({
  authState,
  mergeProblemSummary,
  resetEditorDrafts,
  assignEditorDrafts,
  clearActionError
}){
  const selectedProblemId = ref(0)
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

  function setSelectedProblemSamples(samples){
    if (!selectedProblemDetail.value) {
      return
    }

    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      samples: Array.isArray(samples) ? samples : []
    }
  }

  function applySelectedProblemVersion(problemId, version){
    const normalizedVersion = Number(version)
    if (!Number.isInteger(normalizedVersion) || normalizedVersion <= 0) {
      return
    }

    const selectedProblem = selectedProblemDetail.value
    if (selectedProblem && selectedProblem.problem_id === problemId) {
      selectedProblemDetail.value = {
        ...selectedProblem,
        version: normalizedVersion
      }
    }

    mergeProblemSummary(problemId, {
      version: normalizedVersion
    })
  }

  async function loadSelectedProblem(problemId){
    if (!problemId) {
      return
    }

    selectedProblemId.value = problemId
    clearActionError()
    resetEditorDrafts()

    const result = await detailResource.run(problemId, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return
    }

    mergeProblemSummary(problemId, {
      title: result.data.title,
      version: result.data.version
    })
    assignEditorDrafts(result.data)
  }

  async function selectProblem(problemId, options = {}){
    if (!problemId) {
      return
    }

    if (!options.force && selectedProblemId.value === problemId && selectedProblemDetail.value) {
      return
    }

    await loadSelectedProblem(problemId)
  }

  return {
    isLoadingDetail,
    detailErrorMessage,
    selectedProblemId,
    selectedProblemDetail,
    invalidateSelectedProblemResource,
    setSelectedProblemSamples,
    applySelectedProblemVersion,
    loadSelectedProblem,
    selectProblem
  }
}
