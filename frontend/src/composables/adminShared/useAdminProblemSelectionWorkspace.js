import { computed } from 'vue'

import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useSelectedProblemDetailResource } from '@/composables/adminShared/useSelectedProblemDetailResource'
import { parsePositiveInteger } from '@/utils/parse'

const inactiveWorkspaceResult = Object.freeze({
  status: 'inactive'
})

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
  const pageCallbacks = {
    beforeAllowed: null,
    resetSelectedProblemState: null,
    loadSelectedProblemData: null,
    resetPageState: null
  }
  let isActivated = false
  let hasRegisteredPageWatches = false

  async function runResetSelectedProblemState(){
    if (typeof pageCallbacks.resetSelectedProblemState !== 'function') {
      return inactiveWorkspaceResult
    }

    return pageCallbacks.resetSelectedProblemState({
      problemDetailResource
    })
  }

  async function runLoadSelectedProblemData(problemId = selectedProblemId.value){
    if (typeof pageCallbacks.loadSelectedProblemData !== 'function') {
      return inactiveWorkspaceResult
    }

    return pageCallbacks.loadSelectedProblemData({
      problemId,
      problemDetailResource,
      selectedProblemId
    })
  }

  async function runResetPageState(){
    if (typeof pageCallbacks.resetPageState !== 'function') {
      return inactiveWorkspaceResult
    }

    return pageCallbacks.resetPageState({
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

  async function runAllowedCallbacks(){
    if (!isActivated) {
      return inactiveWorkspaceResult
    }

    if (typeof pageCallbacks.beforeAllowed === 'function') {
      await pageCallbacks.beforeAllowed({
        selectedProblemId,
        query
      })
    }

    const previousSelectedProblemId = selectedProblemId.value

    await query.syncFromRouteAndReload()

    if (selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return runLoadSelectedProblemData()
  }

  async function runDeniedCallbacks(){
    if (!isActivated) {
      return inactiveWorkspaceResult
    }

    return runResetPageState()
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

  function registerPageWatches(){
    if (hasRegisteredPageWatches) {
      return
    }

    pageAccess.watchWhenAllowed(
      () => [
        query.routeSearchMode.value,
        query.routeTitleSearch.value,
        query.routeProblemIdSearch.value
      ],
      () => {
        void query.syncFromRouteAndReload()
      }
    )

    pageAccess.watchWhenAllowed(selectedProblemId, (problemId) => {
      void syncSelectedProblemRouteState(problemId)
    })

    hasRegisteredPageWatches = true
  }

  function replayCurrentAccessState(){
    if (pageAccess.accessState.value === 'initializing') {
      return
    }

    if (pageAccess.accessState.value === 'allowed') {
      void runAllowedCallbacks()
      return
    }

    void runDeniedCallbacks()
  }

  function activate({
    beforeAllowed,
    resetSelectedProblemState,
    loadSelectedProblemData,
    resetPageState
  }){
    if (isActivated) {
      return
    }

    pageCallbacks.beforeAllowed = beforeAllowed
    pageCallbacks.resetSelectedProblemState = resetSelectedProblemState
    pageCallbacks.loadSelectedProblemData = loadSelectedProblemData
    pageCallbacks.resetPageState = resetPageState
    isActivated = true

    registerPageWatches()
    replayCurrentAccessState()
  }

  async function refreshWorkspace(){
    if (!isActivated) {
      return inactiveWorkspaceResult
    }

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
    activate,
    selectProblem,
    loadProblems,
    refreshWorkspace
  }
}
