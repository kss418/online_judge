import { computed } from 'vue'

import { normalizeAdminProblemId } from '@/composables/adminShared/adminProblemSelectionHelpers'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'

export function useAdminProblemsWorkspaceEffects({
  core,
  authState,
  initializeAuth,
  isAuthenticated,
  canManageProblems,
  accessMessages,
  resetSelectedProblemState,
  loadSelectedProblemData,
  resetPageState
}){
  async function syncSelectedProblemRouteState(problemId){
    await resetSelectedProblemState()

    if (!normalizeAdminProblemId(problemId)) {
      return {
        status: 'reset'
      }
    }

    return loadSelectedProblemData(problemId)
  }

  async function handleAllowedAccess(){
    const previousSelectedProblemId = core.selectedProblemId.value

    await core.query.syncFromRouteAndReload()

    if (core.selectedProblemId.value !== previousSelectedProblemId) {
      return
    }

    return loadSelectedProblemData()
  }

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: resetPageState,
    onAllowed: handleAllowedAccess,
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
