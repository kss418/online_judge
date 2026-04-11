import { computed, ref } from 'vue'

import { useRouteQueryState } from '@/composables/useRouteQueryState'
import { parsePositiveInteger } from '@/composables/adminProblems/problemHelpers'

function normalizeSearchMode(value){
  return value === 'problem-id' ? 'problem-id' : 'title'
}

export function useProblemSearchQuery({
  route,
  router,
  formatCount,
  getSelectedProblemId,
  reloadProblems,
  showErrorNotice
}){
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      return {
        selectedProblemId: parsePositiveInteger(query.problemId) ?? 0,
        searchMode: normalizeSearchMode(query.searchMode),
        titleSearch: String(query.searchTitle ?? '').trim(),
        problemIdSearch: parsePositiveInteger(query.searchProblemId)
      }
    },
    buildQuery(state){
      const nextQuery = {}
      const selectedProblemId = Number(state.selectedProblemId ?? 0)
      const searchMode = normalizeSearchMode(state.searchMode)

      if (selectedProblemId > 0) {
        nextQuery.problemId = String(selectedProblemId)
      }

      if (searchMode === 'problem-id') {
        const nextProblemId = parsePositiveInteger(state.problemIdSearch)
        if (nextProblemId != null) {
          nextQuery.searchMode = 'problem-id'
          nextQuery.searchProblemId = String(nextProblemId)
        }

        return nextQuery
      }

      const nextTitle = String(state.titleSearch ?? '').trim()
      if (nextTitle) {
        nextQuery.searchMode = 'title'
        nextQuery.searchTitle = nextTitle
      }

      return nextQuery
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

  const routeSearchMode = computed(() => queryState.routeState.value.searchMode)
  const routeTitleSearch = computed(() => queryState.routeState.value.titleSearch)
  const routeProblemIdSearch = computed(() => queryState.routeState.value.problemIdSearch)
  const preferredProblemIdFromRoute = computed(() => queryState.routeState.value.selectedProblemId)
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

  function syncSearchControlsFromRoute(){
    queryState.syncFromRoute()
  }

  function setSearchMode(nextMode){
    queryState.localState.searchMode.value = normalizeSearchMode(nextMode)
  }

  function handleProblemIdSearchInput(event){
    const normalizedValue = String(event.target?.value ?? '').replace(/\D+/g, '')
    queryState.localState.problemIdSearchInput.value = normalizedValue
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
    }), getSelectedProblemId())
  }

  function resetSearch(){
    queryState.localState.searchMode.value = 'title'
    queryState.localState.titleSearchInput.value = ''
    queryState.localState.problemIdSearchInput.value = ''
    void applySearchQuery(buildSearchState('title'), getSelectedProblemId())
  }

  return {
    searchMode: queryState.localState.searchMode,
    titleSearchInput: queryState.localState.titleSearchInput,
    problemIdSearchInput: queryState.localState.problemIdSearchInput,
    preferredProblemIdFromRoute,
    routeSearchMode,
    routeTitleSearch,
    routeProblemIdSearch,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    syncSearchControlsFromRoute,
    setSearchMode,
    handleProblemIdSearchInput,
    submitSearch,
    resetSearch
  }
}
