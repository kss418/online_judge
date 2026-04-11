import { computed } from 'vue'

import { getProblemList } from '@/api/problem'
import { useAsyncResource } from '@/composables/useAsyncResource'

export function useProblemListResource({
  authState,
  canManageProblems,
  routeSearchMode,
  routeTitleSearch,
  routeProblemIdSearch,
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
    async load({ activeTitleSearch, activeProblemIdSearch }){
      const response = await getProblemList({
        title: activeTitleSearch,
        bearerToken: authState.token
      })
      const responseProblems = Array.isArray(response.problems) ? response.problems : []
      const filteredProblems = activeProblemIdSearch == null
        ? responseProblems
        : responseProblems.filter((problem) => Number(problem.problem_id ?? 0) === activeProblemIdSearch)

      return filteredProblems.map((problem) => ({
        problem_id: Number(problem.problem_id ?? 0),
        title: problem.title ?? '',
        version: Number(problem.version ?? 0),
        time_limit_ms: Number(problem.time_limit_ms ?? 0),
        memory_limit_mb: Number(problem.memory_limit_mb ?? 0)
      }))
    },
    getErrorMessage(error){
      return error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
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
    const activeTitleSearch = routeSearchMode.value === 'title'
      ? routeTitleSearch.value
      : ''
    const activeProblemIdSearch = routeSearchMode.value === 'problem-id'
      ? routeProblemIdSearch.value
      : null

    clearActionError()

    const result = await problemListResource.run({
      activeTitleSearch,
      activeProblemIdSearch
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
