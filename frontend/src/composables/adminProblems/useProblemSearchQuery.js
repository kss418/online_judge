import { computed, ref } from 'vue'

import { useAdminProblemSearchControls } from '@/composables/useAdminProblemSearchControls'
import { useRouteQueryState } from '@/composables/useRouteQueryState'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  normalizeSearchMode,
  parsePositiveInteger,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'

export function useProblemSearchQuery({
  route,
  router,
  formatCount,
  selectedProblemId,
  reloadProblems,
  showErrorNotice
}){
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      return parseProblemAdminSearchRouteQuery(query, {
        includeSelectedProblemId: true
      })
    },
    buildQuery(state){
      return buildProblemAdminSearchRouteQuery(state, {
        includeSelectedProblemId: true
      })
    },
    createLocalState(){
      return {
        searchMode: ref('title'),
        titleSearchInput: ref(''),
        problemIdSearchInput: ref('')
      }
    },
    syncLocalState(localState, state){
      localState.searchMode.value = state.searchMode
      localState.titleSearchInput.value = state.titleSearch
      localState.problemIdSearchInput.value = state.problemIdSearch != null
        ? String(state.problemIdSearch)
        : ''
    }
  })
  const preferredProblemIdFromRoute = computed(() => queryState.routeState.value.selectedProblemId)

  function buildSearchState(mode, options = {}){
    const nextMode = normalizeSearchMode(mode)

    return {
      selectedProblemId: queryState.routeState.value.selectedProblemId,
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
    searchMode: queryState.localState.searchMode,
    titleSearchInput: queryState.localState.titleSearchInput,
    problemIdSearchInput: queryState.localState.problemIdSearchInput,
    preferredProblemIdFromRoute,
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
