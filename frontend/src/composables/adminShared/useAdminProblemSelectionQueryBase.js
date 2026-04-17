import { computed, readonly, ref } from 'vue'

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

  const routeSearchMode = computed(() => queryState.routeState.value.searchMode)
  const routeTitleSearch = computed(() => queryState.routeState.value.titleSearch)
  const routeProblemIdSearch = computed(() => queryState.routeState.value.problemIdSearch)
  const hasAppliedSearch = computed(() => {
    if (routeSearchMode.value === 'problem-id') {
      return routeProblemIdSearch.value != null
    }

    return Boolean(routeTitleSearch.value)
  })
  const problemListCaption = computed(() => {
    if (routeSearchMode.value === 'problem-id' && routeProblemIdSearch.value != null) {
      return `문제 #${formatCount(routeProblemIdSearch.value)} 검색 결과`
    }

    if (routeTitleSearch.value) {
      return `"${routeTitleSearch.value}" 검색 결과`
    }

    return '전체 문제'
  })
  const emptyProblemListMessage = computed(() => {
    if (routeSearchMode.value === 'problem-id' && routeProblemIdSearch.value != null) {
      return `문제 #${formatCount(routeProblemIdSearch.value)}를 찾지 못했습니다.`
    }

    if (routeTitleSearch.value) {
      return '검색 조건에 맞는 문제가 없습니다.'
    }

    return '등록된 문제가 아직 없습니다.'
  })
  const preferredProblemIdForReload = computed(() => (
    routeSearchMode.value === 'problem-id'
      ? (routeProblemIdSearch.value || resolveSelectedProblemId(selectedProblemId))
      : resolveSelectedProblemId(selectedProblemId)
  ))

  function setSearchMode(nextMode){
    queryState.localState.searchMode.value = normalizeSearchMode(nextMode)
  }

  function updateTitleSearchInput(value){
    queryState.localState.titleSearchInput.value = String(value ?? '')
  }

  function updateProblemIdSearchInput(value){
    const normalizedValue = String(value ?? '').replace(/\D+/g, '')
    queryState.localState.problemIdSearchInput.value = normalizedValue
  }

  function resetSearchControls(){
    queryState.localState.searchMode.value = 'title'
    queryState.localState.titleSearchInput.value = ''
    queryState.localState.problemIdSearchInput.value = ''
  }

  async function applySearchQuery(nextState, preferredProblemId){
    const didNavigate = await queryState.navigate(nextState)

    if (!didNavigate) {
      await reloadProblems(preferredProblemId)
    }
  }

  function submitSearch(){
    if (queryState.localState.searchMode.value === 'problem-id') {
      const nextProblemId = parsePositiveInteger(queryState.localState.problemIdSearchInput.value)
      if (nextProblemId == null) {
        showErrorNotice('문제 번호를 입력하세요.')
        return
      }

      void applySearchQuery(buildSearchState('problem-id', {
        problemId: nextProblemId
      }), nextProblemId)
      return
    }

    void applySearchQuery(buildSearchState('title', {
      title: queryState.localState.titleSearchInput.value
    }), resolveSelectedProblemId(selectedProblemId))
  }

  function resetSearch(){
    resetSearchControls()
    void applySearchQuery(buildSearchState('title'), resolveSelectedProblemId(selectedProblemId))
  }

  async function replaceSelectedProblem(problemId, options = {}){
    const nextSelectedProblemId = normalizeProblemId(problemId)

    return queryState.navigate(queryState.routeState.value, {
      mode: options.push ? 'push' : 'replace',
      query: options.query,
      selectedProblemId: nextSelectedProblemId
    })
  }

  async function selectCreatedProblem(problemId){
    const nextSelectedProblemId = normalizeProblemId(problemId)
    const nextState = buildSearchState('title')

    resetSearchControls()

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
    queryState.syncFromRoute()
    return reloadProblems(preferredProblemIdForReload.value)
  }

  return {
    searchMode: readonly(queryState.localState.searchMode),
    titleSearchInput: readonly(queryState.localState.titleSearchInput),
    problemIdSearchInput: readonly(queryState.localState.problemIdSearchInput),
    routeSearchMode,
    routeTitleSearch,
    routeProblemIdSearch,
    routeState: queryState.routeState,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    preferredProblemIdForReload,
    syncFromRouteAndReload,
    setSearchMode,
    updateTitleSearchInput,
    updateProblemIdSearchInput,
    resetSearchControls,
    replaceSelectedProblem,
    selectCreatedProblem,
    submitSearch,
    resetSearch
  }
}
