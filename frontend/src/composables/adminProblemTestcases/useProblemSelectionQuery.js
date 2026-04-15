import { computed } from 'vue'

import { useAdminProblemSelectionQueryBase } from '@/composables/adminShared/useAdminProblemSelectionQueryBase'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
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
  const query = useAdminProblemSelectionQueryBase({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems,
    showErrorNotice,
    parseQuery: parseProblemAdminSearchRouteQuery,
    buildQuery: buildProblemAdminSearchRouteQuery,
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

  async function replaceProblemRoute(problemId, options = {}){
    const method = options.push ? 'push' : 'replace'
    const nextQuery = options.query ?? query.buildCanonicalQuery(query.routeState.value)

    await router[method]({
      name: 'admin-problem-testcases',
      params: problemId > 0
        ? {
          problemId: String(problemId)
        }
        : {},
      query: nextQuery
    })
  }

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
    searchMode: query.searchMode,
    titleSearchInput: query.titleSearchInput,
    problemIdSearchInput: query.problemIdSearchInput,
    routeSearchMode: query.routeSearchMode,
    routeTitleSearch: query.routeTitleSearch,
    routeProblemIdSearch: query.routeProblemIdSearch,
    routeState: query.routeState,
    hasAppliedSearch: query.hasAppliedSearch,
    problemListCaption: query.problemListCaption,
    emptyProblemListMessage: query.emptyProblemListMessage,
    preferredProblemIdForReload: query.preferredProblemIdForReload,
    syncSearchControlsFromRoute: query.syncSearchControlsFromRoute,
    setSearchMode: query.setSearchMode,
    handleProblemIdSearchInput: query.handleProblemIdSearchInput,
    replaceProblemRoute,
    selectProblem,
    submitSearch: query.submitSearch,
    resetSearch: query.resetSearch
  }
}
