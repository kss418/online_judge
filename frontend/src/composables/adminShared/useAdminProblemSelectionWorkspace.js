import { computed } from 'vue'

import {
  normalizeAdminProblemId
} from '@/composables/adminShared/adminProblemSelectionHelpers'
import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useSelectedProblemDetailResource } from '@/composables/adminShared/useSelectedProblemDetailResource'

export function useAdminProblemSelectionWorkspaceCore({
  route,
  router,
  routeName,
  authState,
  canManageProblems,
  showErrorNotice,
  formatCount
}){
  const selectedProblemId = computed(() => normalizeAdminProblemId(route.params.problemId))

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return {
        status: 'blocked'
      }
    }

    const preferredProblemId = normalizeAdminProblemId(
      options.preferredProblemId ?? selectedProblemId.value
    )
    const result = await problemCatalogResource.loadProblems()

    if (result.status !== 'success') {
      return result
    }

    if (!problemCatalogResource.problems.value.length) {
      if (selectedProblemId.value > 0) {
        await query.replaceSelectedProblem(0)
      }

      return result
    }

    const nextProblemId = problemCatalogResource.problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problemCatalogResource.problems.value[0].problem_id

    if (nextProblemId > 0 && nextProblemId !== selectedProblemId.value) {
      await query.replaceSelectedProblem(nextProblemId)
    }

    return result
  }

  const query = useAdminProblemCatalogQuery({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems: async (preferredProblemId) => {
      await loadProblems({ preferredProblemId })
    },
    showErrorNotice,
    buildLocation({ query: nextQuery, selectedProblemId: nextSelectedProblemId }){
      const routeProblemId = typeof nextSelectedProblemId === 'undefined'
        ? selectedProblemId.value
        : normalizeAdminProblemId(nextSelectedProblemId)

      return {
        name: routeName,
        params: routeProblemId > 0
          ? {
            problemId: String(routeProblemId)
          }
          : {},
        query: nextQuery
      }
    }
  })
  const problemCatalogResource = useAdminProblemCatalogResource({
    authState,
    routeQueryState: query.routeState
  })
  const problemDetailResource = useSelectedProblemDetailResource({
    authState,
    selectedProblemId,
    mergeProblemSummary: problemCatalogResource.mergeProblemSummary
  })

  async function selectProblem(problemId){
    const normalizedProblemId = normalizeAdminProblemId(problemId)
    if (!normalizedProblemId || normalizedProblemId === selectedProblemId.value) {
      return
    }

    await query.replaceSelectedProblem(normalizedProblemId, {
      push: true
    })
  }

  return {
    selectedProblemId,
    query,
    problemCatalogResource,
    problemDetailResource,
    selectProblem,
    loadProblems
  }
}
