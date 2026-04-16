import { computed } from 'vue'

import { getProblemList } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { buildApiQuery as buildProblemAdminSearchApiQuery } from '@/queryState/problemAdminSearch'
import { formatApiError } from '@/utils/apiError'

export function useAdminProblemCatalogResource({
  authState,
  routeQueryState
}){
  const problemListResource = useAsyncResource({
    initialData: [],
    async load({ routeQuery }){
      const apiQuery = buildProblemAdminSearchApiQuery(routeQuery)
      const response = await getProblemList({
        title: apiQuery.title,
        problemId: apiQuery.problemId,
        bearerToken: authState.token || ''
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

  async function loadProblems(){
    return problemListResource.run({
      routeQuery: routeQueryState.value
    }, {
      resetDataOnError: true
    })
  }

  function resetProblems(){
    problemListResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  return {
    isLoadingProblems,
    listErrorMessage,
    problems,
    problemCount,
    mergeProblemSummary,
    loadProblems,
    resetProblems
  }
}
