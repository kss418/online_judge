import { computed } from 'vue'

import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useSelectedProblemDetailResource } from '@/composables/adminShared/useSelectedProblemDetailResource'
import { parsePositiveInteger } from '@/utils/parse'

function canRefreshWorkspace(canRefresh){
  if (typeof canRefresh === 'function') {
    return Boolean(canRefresh())
  }

  if (canRefresh && typeof canRefresh === 'object' && 'value' in canRefresh) {
    return Boolean(canRefresh.value)
  }

  if (typeof canRefresh === 'undefined') {
    return true
  }

  return Boolean(canRefresh)
}

export function useAdminProblemSelectionWorkspaceCore({
  route,
  router,
  routeName,
  authState,
  canManageProblems,
  showErrorNotice,
  formatCount
}){
  const selectedProblemId = computed(() => parsePositiveInteger(route.params.problemId) ?? 0)

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return {
        status: 'blocked'
      }
    }

    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
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
      const routeProblemId = Number.isInteger(Number(nextSelectedProblemId))
        ? Number(nextSelectedProblemId)
        : selectedProblemId.value

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
    const normalizedProblemId = parsePositiveInteger(problemId)
    if (normalizedProblemId == null || normalizedProblemId === selectedProblemId.value) {
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

export function useAdminProblemSelectionWorkspaceEffects({
  core,
  authState,
  initializeAuth,
  isAuthenticated,
  canManageProblems,
  accessMessages,
  canRefresh,
  resetSelectedProblemState,
  loadSelectedProblemData,
  resetPageState
}){
  async function syncSelectedProblemRouteState(problemId){
    await resetSelectedProblemState()

    if ((parsePositiveInteger(problemId) ?? 0) <= 0) {
      return {
        status: 'reset'
      }
    }

    return loadSelectedProblemData(problemId)
  }

  async function runAllowedCallbacks(){
    const previousSelectedProblemId = core.selectedProblemId.value

    await core.query.syncFromRouteAndReload()

    if (core.selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return loadSelectedProblemData()
  }

  async function runDeniedCallbacks(){
    return resetPageState()
  }

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: runDeniedCallbacks,
    onAllowed: runAllowedCallbacks,
    loggedOutMessage: accessMessages.loggedOutMessage,
    deniedMessage: accessMessages.deniedMessage
  })

  pageAccess.watchWhenAllowed(
    () => [
      core.query.routeSearchMode.value,
      core.query.routeTitleSearch.value,
      core.query.routeProblemIdSearch.value
    ],
    () => {
      void core.query.syncFromRouteAndReload()
    }
  )

  pageAccess.watchWhenAllowed(core.selectedProblemId, (problemId) => {
    void syncSelectedProblemRouteState(problemId)
  })

  async function refreshWorkspace(){
    if (!canRefreshWorkspace(canRefresh)) {
      return
    }

    const previousSelectedProblemId = core.selectedProblemId.value

    await core.loadProblems({
      preferredProblemId: core.query.preferredProblemIdForReload.value
    })

    if (core.selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return loadSelectedProblemData()
  }

  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || core.problemCatalogResource.isLoadingProblems.value
  )
  const shell = computed(() => ({
    state: pageAccess.accessState.value,
    message: pageAccess.accessMessage.value
  }))

  return {
    pageAccess,
    isLoadingProblems,
    shell,
    refreshWorkspace
  }
}
