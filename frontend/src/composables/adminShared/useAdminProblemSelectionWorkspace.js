import { computed } from 'vue'

import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useAdminProblemRouteCatalogReload } from '@/composables/adminShared/useAdminProblemRouteCatalogReload'
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

export function useAdminProblemSelectionWorkspace({
  route,
  router,
  routeName,
  authState,
  initializeAuth,
  isAuthenticated,
  canManageProblems,
  showErrorNotice,
  formatCount,
  accessMessages,
  beforeAllowed,
  resetSelectedProblemState,
  loadSelectedProblemData,
  resetPageState,
  canRefresh
}){
  const selectedProblemId = computed(() => parsePositiveInteger(route.params.problemId) ?? 0)

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

  async function runResetSelectedProblemState(){
    return resetSelectedProblemState({
      problemDetailResource
    })
  }

  async function runLoadSelectedProblemData(problemId = selectedProblemId.value){
    return loadSelectedProblemData({
      problemId,
      problemDetailResource,
      selectedProblemId
    })
  }

  async function runResetPageState(){
    return resetPageState({
      query,
      problemCatalogResource,
      problemDetailResource
    })
  }

  async function selectProblem(problemId){
    const normalizedProblemId = parsePositiveInteger(problemId)
    if (normalizedProblemId == null || normalizedProblemId === selectedProblemId.value) {
      return
    }

    await query.replaceSelectedProblem(normalizedProblemId, {
      push: true
    })
  }

  async function syncSelectedProblemRouteState(problemId){
    await runResetSelectedProblemState()

    if ((parsePositiveInteger(problemId) ?? 0) <= 0) {
      return {
        status: 'reset'
      }
    }

    return runLoadSelectedProblemData(problemId)
  }

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
      } else {
        await runResetSelectedProblemState()
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

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: runResetPageState,
    async onAllowed(){
      if (typeof beforeAllowed === 'function') {
        await beforeAllowed({
          selectedProblemId,
          query
        })
      }

      const previousSelectedProblemId = selectedProblemId.value

      await query.syncFromRouteAndReload()

      if (selectedProblemId.value !== previousSelectedProblemId) {
        return
      }

      await runLoadSelectedProblemData()
    },
    loggedOutMessage: accessMessages.loggedOutMessage,
    deniedMessage: accessMessages.deniedMessage
  })

  useAdminProblemRouteCatalogReload({
    pageAccess,
    query
  })

  pageAccess.watchWhenAllowed(selectedProblemId, (problemId) => {
    void syncSelectedProblemRouteState(problemId)
  })

  async function refreshWorkspace(){
    if (!canRefreshWorkspace(canRefresh)) {
      return
    }

    const previousSelectedProblemId = selectedProblemId.value

    await loadProblems({
      preferredProblemId: query.preferredProblemIdForReload.value
    })

    if (selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return runLoadSelectedProblemData()
  }

  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || problemCatalogResource.isLoadingProblems.value
  )
  const shell = computed(() => ({
    state: pageAccess.accessState.value,
    message: pageAccess.accessMessage.value
  }))

  return {
    selectedProblemId,
    query,
    problemCatalogResource,
    problemDetailResource,
    pageAccess,
    isLoadingProblems,
    shell,
    selectProblem,
    loadProblems,
    refreshWorkspace
  }
}
