import { computed, ref } from 'vue'

import { useAdminProblemSearchControls } from '@/composables/useAdminProblemSearchControls'
import { useRouteQueryState } from '@/composables/useRouteQueryState'
import { normalizeSearchMode } from '@/queryState/problemAdminSearch'
import { parsePositiveInteger } from '@/utils/parse'

function createSearchLocalState(){
  return {
    searchMode: ref('title'),
    titleSearchInput: ref(''),
    problemIdSearchInput: ref('')
  }
}

function syncSearchLocalState(localState, state){
  localState.searchMode.value = state.searchMode
  localState.titleSearchInput.value = state.titleSearch
  localState.problemIdSearchInput.value = state.problemIdSearch != null
    ? String(state.problemIdSearch)
    : ''
}

export function useAdminProblemSelectionQueryBase({
  route,
  router,
  formatCount,
  selectedProblemId,
  reloadProblems,
  showErrorNotice,
  parseQuery,
  buildQuery,
  buildLocation
}){
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery,
    buildQuery,
    createLocalState: createSearchLocalState,
    syncLocalState: syncSearchLocalState,
    buildLocation
  })

  function buildSearchState(mode, options = {}){
    const nextMode = normalizeSearchMode(mode)

    return {
      ...queryState.routeState.value,
      searchMode: nextMode,
      titleSearch: nextMode === 'problem-id'
        ? ''
        : String(options.title ?? '').trim(),
      problemIdSearch: nextMode === 'problem-id'
        ? parsePositiveInteger(options.problemId)
        : null
    }
  }

  const searchControls = useAdminProblemSearchControls({
    queryState,
    selectedProblemId,
    formatCount,
    reloadProblems,
    showErrorNotice,
    createSearchState: buildSearchState
  })

  return {
    queryState,
    buildCanonicalQuery: queryState.buildCanonicalQuery,
    searchMode: queryState.localState.searchMode,
    titleSearchInput: queryState.localState.titleSearchInput,
    problemIdSearchInput: queryState.localState.problemIdSearchInput,
    preferredProblemIdFromRoute: computed(() =>
      parsePositiveInteger(queryState.routeState.value.selectedProblemId) ?? 0
    ),
    routeSearchMode: searchControls.routeSearchMode,
    routeTitleSearch: searchControls.routeTitleSearch,
    routeProblemIdSearch: searchControls.routeProblemIdSearch,
    routeState: queryState.routeState,
    hasAppliedSearch: searchControls.hasAppliedSearch,
    problemListCaption: searchControls.problemListCaption,
    emptyProblemListMessage: searchControls.emptyProblemListMessage,
    preferredProblemIdForReload: searchControls.preferredProblemIdForReload,
    syncSearchControlsFromRoute: searchControls.syncSearchControlsFromRoute,
    setSearchMode: searchControls.setSearchMode,
    handleProblemIdSearchInput: searchControls.handleProblemIdSearchInput,
    submitSearch: searchControls.submitSearch,
    resetSearch: searchControls.resetSearch
  }
}
