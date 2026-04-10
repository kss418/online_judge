import { ref } from 'vue'

import { getProblemDetail } from '@/api/problem'
import { normalizeProblemDetail, normalizeProblemSamples } from '@/composables/adminProblems/problemHelpers'

export function useProblemDetailResource({
  authState,
  mergeProblemSummary,
  resetEditorDrafts,
  assignEditorDrafts,
  clearActionError
}){
  const isLoadingDetail = ref(false)
  const detailErrorMessage = ref('')
  const selectedProblemId = ref(0)
  const selectedProblemDetail = ref(null)

  let latestProblemDetailRequestId = 0

  function setSelectedProblemSamples(samples){
    if (!selectedProblemDetail.value) {
      return
    }

    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      samples: normalizeProblemSamples(samples)
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

    const requestId = ++latestProblemDetailRequestId
    selectedProblemId.value = problemId
    isLoadingDetail.value = true
    detailErrorMessage.value = ''
    clearActionError()
    selectedProblemDetail.value = null
    resetEditorDrafts()

    try {
      const response = await getProblemDetail(problemId, {
        bearerToken: authState.token || ''
      })

      if (requestId !== latestProblemDetailRequestId) {
        return
      }

      const normalizedDetail = normalizeProblemDetail(response)
      selectedProblemDetail.value = normalizedDetail
      mergeProblemSummary(problemId, {
        title: normalizedDetail.title,
        version: normalizedDetail.version
      })
      assignEditorDrafts(normalizedDetail)
    } catch (error) {
      if (requestId !== latestProblemDetailRequestId) {
        return
      }

      detailErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 정보를 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemDetailRequestId) {
        isLoadingDetail.value = false
      }
    }
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
    setSelectedProblemSamples,
    applySelectedProblemVersion,
    loadSelectedProblem,
    selectProblem
  }
}
