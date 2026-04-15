import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { parsePositiveInteger } from '@/utils/parse'

function resolveSourceValue(source){
  if (typeof source === 'function') {
    return source()
  }

  if (source && typeof source === 'object' && 'value' in source) {
    return source.value
  }

  return source
}

function normalizeProblemId(value){
  return parsePositiveInteger(value) ?? 0
}

export function useAdminProblemWorkspaceBase({
  authState,
  initializeAuth,
  isAuthenticated,
  hasAccess,
  loggedOutMessage,
  deniedMessage,
  routeSearchSources = [],
  syncSearchControlsFromRoute,
  loadProblems,
  getInitialPreferredProblemId,
  getRefreshPreferredProblemId,
  resetPageState,
  afterInitialProblemLoad,
  afterRefreshProblemLoad,
  preferredProblemIdSource,
  onPreferredProblemIdChange,
  selectedProblemIdSource,
  resetSelectedProblemState,
  reloadSelectedProblemData,
  clearSelectedProblemState,
  isRefreshBlocked
}){
  async function reloadProblems(preferredProblemId){
    return loadProblems({
      preferredProblemId: normalizeProblemId(preferredProblemId)
    })
  }

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess,
    onDenied: resetPageState,
    async onAllowed(){
      syncSearchControlsFromRoute?.()
      await reloadProblems(getInitialPreferredProblemId?.())
      await afterInitialProblemLoad?.()
    },
    loggedOutMessage,
    deniedMessage
  })

  if (preferredProblemIdSource) {
    pageAccess.watchWhenAllowed(preferredProblemIdSource, (problemId) => {
      const nextProblemId = normalizeProblemId(problemId)
      if (nextProblemId <= 0) {
        return
      }

      if (typeof onPreferredProblemIdChange === 'function') {
        void onPreferredProblemIdChange(nextProblemId)
        return
      }

      void reloadProblems(nextProblemId)
    })
  }

  if (routeSearchSources.length > 0) {
    pageAccess.watchWhenAllowed(
      () => routeSearchSources.map((source) => resolveSourceValue(source)),
      () => {
        syncSearchControlsFromRoute?.()
        void reloadProblems(getRefreshPreferredProblemId?.())
      }
    )
  }

  if (selectedProblemIdSource && typeof resetSelectedProblemState === 'function') {
    pageAccess.watchWhenAllowed(selectedProblemIdSource, (problemId) => {
      resetSelectedProblemState()

      if (normalizeProblemId(problemId) > 0) {
        if (typeof reloadSelectedProblemData === 'function') {
          void reloadSelectedProblemData(problemId)
        }
        return
      }

      clearSelectedProblemState?.()
    })
  }

  async function refreshWorkspace(){
    if (isRefreshBlocked?.()) {
      return
    }

    await reloadProblems(getRefreshPreferredProblemId?.())
    await afterRefreshProblemLoad?.()
  }

  return {
    pageAccess,
    refreshWorkspace
  }
}
