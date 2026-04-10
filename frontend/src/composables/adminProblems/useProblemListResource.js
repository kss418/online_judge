import { computed, ref } from 'vue'

import { getProblemList } from '@/api/problem'

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
  selectProblem
}){
  const isLoadingProblems = ref(true)
  const listErrorMessage = ref('')
  const problems = ref([])
  const problemCount = computed(() => problems.value.length)

  let latestProblemListRequestId = 0

  function mergeProblemSummary(problemId, patch){
    problems.value = problems.value.map((problem) =>
      problem.problem_id === problemId
        ? {
          ...problem,
          ...patch
        }
        : problem
    )
  }

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return
    }

    const requestId = ++latestProblemListRequestId
    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
    isLoadingProblems.value = true
    listErrorMessage.value = ''
    clearActionError()

    try {
      const activeTitleSearch = routeSearchMode.value === 'title'
        ? routeTitleSearch.value
        : ''
      const activeProblemIdSearch = routeSearchMode.value === 'problem-id'
        ? routeProblemIdSearch.value
        : null
      const response = await getProblemList({
        title: activeTitleSearch,
        bearerToken: authState.token
      })

      if (requestId !== latestProblemListRequestId) {
        return
      }

      const responseProblems = Array.isArray(response.problems) ? response.problems : []
      const filteredProblems = activeProblemIdSearch == null
        ? responseProblems
        : responseProblems.filter((problem) => Number(problem.problem_id ?? 0) === activeProblemIdSearch)

      problems.value = filteredProblems.map((problem) => ({
        problem_id: Number(problem.problem_id ?? 0),
        title: problem.title ?? '',
        version: Number(problem.version ?? 0),
        time_limit_ms: Number(problem.time_limit_ms ?? 0),
        memory_limit_mb: Number(problem.memory_limit_mb ?? 0)
      }))

      if (!problems.value.length) {
        selectedProblemId.value = 0
        selectedProblemDetail.value = null
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
    } catch (error) {
      if (requestId !== latestProblemListRequestId) {
        return
      }

      problems.value = []
      selectedProblemId.value = 0
      selectedProblemDetail.value = null
      resetEditorDrafts()
      listErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemListRequestId) {
        isLoadingProblems.value = false
      }
    }
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
