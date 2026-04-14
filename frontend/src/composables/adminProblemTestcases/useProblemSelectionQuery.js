import { computed, ref } from 'vue'

import { useAdminProblemSearchControls } from '@/composables/useAdminProblemSearchControls'
import { useRouteQueryState } from '@/composables/useRouteQueryState'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  normalizeSearchMode,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemSelectionQuery({
  route,
  router,
  formatCount,
  reloadProblems,
  showErrorNotice
}){
  const selectedProblemId = computed(() => parsePositiveInteger(route.params.problemId) ?? 0)
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery: parseProblemAdminSearchRouteQuery,
    buildQuery: buildProblemAdminSearchRouteQuery,
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
    },
    buildLocation({ query }){
      return {
        name: 'admin-problem-testcases',
        params: selectedProblemId.value > 0
          ? {
            problemId: String(selectedProblemId.value)
          }
          : {},
        query
      }
    }
  })

  function buildSearchState(mode, options = {}){
    const nextMode = normalizeSearchMode(mode)

    return {
      searchMode: nextMode,
      titleSearch: nextMode === 'problem-id'
        ? ''
        : String(options.title ?? '').trim(),
      problemIdSearch: nextMode === 'problem-id'
        ? parsePositiveInteger(options.problemId)
        : null
    }
  }

  async function replaceProblemRoute(problemId, options = {}){
    const method = options.push ? 'push' : 'replace'
    const query = options.query ?? queryState.buildCanonicalQuery(queryState.routeState.value)

    await router[method]({
      name: 'admin-problem-testcases',
      params: problemId > 0
        ? {
          problemId: String(problemId)
        }
        : {},
      query
    })
  }
  const searchControls = useAdminProblemSearchControls({
    queryState,
    selectedProblemId,
    formatCount,
    reloadProblems,
    showErrorNotice,
    createSearchState: buildSearchState
  })

  async function selectProblem(problemId){
    if (problemId === selectedProblemId.value) {
      return
    }

    await replaceProblemRoute(problemId, {
      push: true
    })
  }

  return {
    selectedProblemId,
    searchMode: queryState.localState.searchMode,
    titleSearchInput: queryState.localState.titleSearchInput,
    problemIdSearchInput: queryState.localState.problemIdSearchInput,
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
    replaceProblemRoute,
    selectProblem,
    submitSearch: searchControls.submitSearch,
    resetSearch: searchControls.resetSearch
  }
}
