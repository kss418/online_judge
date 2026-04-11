import { computed } from 'vue'

import { getProblemList } from '@/api/problem'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { buildApiQuery as buildProblemAdminSearchApiQuery } from '@/queryState/problemAdminSearch'
import { formatApiError } from '@/utils/apiError'

export function useProblemListResource({
  authState,
  canManageProblems,
  routeQueryState,
  selectedProblemId,
  selectedProblemDetail,
  detailErrorMessage,
  resetEditorDrafts,
  clearActionError,
  invalidateSelectedProblemResource,
  selectProblem
}){
  const problemListResource = useAsyncResource({
    initialData: [],
    async load({ routeQuery }){
      const apiQuery = buildProblemAdminSearchApiQuery(routeQuery)
      const response = await getProblemList({
        title: apiQuery.title,
        problemId: apiQuery.problemId,
        bearerToken: authState.token
      })

      return response.problems
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 목록을 불러오지 못했습니다.'
      })
    }
  })

  const isLoadingProblems = problemListResource.isLoading
  const listErrorMessage = problemListResource.errorMessage
  const problems = problemListResource.data
  const problemCount = computed(() => problems.value.length)

  function mergeProblemSummary(problemId, patch){
    problemListResource.mutate((problemItems) =>
      problemItems.map((problem) =>
        problem.problem_id === problemId
          ? {
            ...problem,
            ...patch
          }
          : problem
      )
    )
  }

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return
    }

    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)

    clearActionError()

    const result = await problemListResource.run({
      routeQuery: routeQueryState.value
    }, {
      resetDataOnError: true
    })

    if (result.status === 'error') {
      selectedProblemId.value = 0
      invalidateSelectedProblemResource()
      detailErrorMessage.value = ''
      resetEditorDrafts()
      return
    }

    if (result.status !== 'success') {
      return
    }

    if (!problems.value.length) {
      selectedProblemId.value = 0
      invalidateSelectedProblemResource()
      detailErrorMessage.value = ''
      resetEditorDrafts()
      return
    }

    const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problems.value[0].problem_id

    void selectProblem(nextProblemId, {
      force: nextProblemId !== selectedProblemId.value || !selectedProblemDetail.value
    })
  }

  return {
    isLoadingProblems,
    listErrorMessage,
    problems,
    problemCount,
    mergeProblemSummary,
    loadProblems
  }
}
