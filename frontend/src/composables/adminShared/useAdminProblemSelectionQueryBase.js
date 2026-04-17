import { computed, readonly, ref } from 'vue'

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

function resolveSelectedProblemId(selectedProblemId){
  if (selectedProblemId && typeof selectedProblemId === 'object' && 'value' in selectedProblemId) {
    return normalizeProblemId(selectedProblemId.value)
  }

  return normalizeProblemId(selectedProblemId)
}

function normalizeProblemId(problemId){
  const normalizedProblemId = Number(problemId)
  return Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
    ? normalizedProblemId
    : 0
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
  const preferredProblemIdFromRoute = computed(() =>
    parsePositiveInteger(queryState.routeState.value.selectedProblemId) ?? 0
  )
  const preferredProblemIdForReload = computed(() => (
    searchControls.routeSearchMode.value === 'problem-id'
      ? (searchControls.routeProblemIdSearch.value || resolveSelectedProblemId(selectedProblemId))
      : (preferredProblemIdFromRoute.value || resolveSelectedProblemId(selectedProblemId))
  ))

  function buildSelectionState(problemId){
    return {
      ...queryState.routeState.value,
      selectedProblemId: normalizeProblemId(problemId)
    }
  }

  async function replaceSelectedProblem(problemId, options = {}){
    const nextSelectedProblemId = normalizeProblemId(problemId)
    const nextState = buildSelectionState(nextSelectedProblemId)

    return queryState.navigate(nextState, {
      mode: options.push ? 'push' : 'replace',
      query: options.query,
      selectedProblemId: nextSelectedProblemId
    })
  }

  async function selectCreatedProblem(problemId){
    const nextSelectedProblemId = normalizeProblemId(problemId)
    const nextState = {
      ...buildSearchState('title'),
      selectedProblemId: nextSelectedProblemId
    }

    searchControls.resetSearchControls()

    const didNavigate = await queryState.navigate(nextState, {
      mode: 'replace',
      selectedProblemId: nextSelectedProblemId
    })

    if (!didNavigate) {
      await reloadProblems(nextSelectedProblemId)
    }

    return didNavigate
  }

  async function syncFromRouteAndReload(){
    searchControls.syncSearchControlsFromRoute()
    await reloadProblems(preferredProblemIdForReload.value)
  }

  return {
    queryState,
    buildCanonicalQuery: queryState.buildCanonicalQuery,
    searchMode: readonly(queryState.localState.searchMode),
    titleSearchInput: readonly(queryState.localState.titleSearchInput),
    problemIdSearchInput: readonly(queryState.localState.problemIdSearchInput),
    preferredProblemIdFromRoute,
    routeSearchMode: searchControls.routeSearchMode,
    routeTitleSearch: searchControls.routeTitleSearch,
    routeProblemIdSearch: searchControls.routeProblemIdSearch,
    routeState: queryState.routeState,
    hasAppliedSearch: searchControls.hasAppliedSearch,
    problemListCaption: searchControls.problemListCaption,
    emptyProblemListMessage: searchControls.emptyProblemListMessage,
    preferredProblemIdForReload,
    syncSearchControlsFromRoute: searchControls.syncSearchControlsFromRoute,
    syncFromRouteAndReload,
    setSearchMode: searchControls.setSearchMode,
    updateTitleSearchInput: searchControls.updateTitleSearchInput,
    updateProblemIdSearchInput: searchControls.updateProblemIdSearchInput,
    resetSearchControls: searchControls.resetSearchControls,
    replaceSelectedProblem,
    selectCreatedProblem,
    submitSearch: searchControls.submitSearch,
    resetSearch: searchControls.resetSearch
  }
}
