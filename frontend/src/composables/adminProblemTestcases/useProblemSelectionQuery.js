import { computed, ref } from 'vue'

import { useRouteQueryState } from '@/composables/useRouteQueryState'
import { parsePositiveInteger } from '@/composables/adminProblemTestcases/testcaseHelpers'

function normalizeSearchMode(value){
  return value === 'problem-id' ? 'problem-id' : 'title'
}

export function useProblemSelectionQuery({
  route,
  router,
  formatCount,
  getSelectedProblemId,
  reloadProblems,
  showErrorNotice
}){
  const selectedProblemId = computed(() => {
    const parsedValue = Number.parseInt(route.params.problemId, 10)
    return Number.isInteger(parsedValue) && parsedValue > 0 ? parsedValue : 0
  })
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      return {
        searchMode: normalizeSearchMode(query.searchMode),
        titleSearch: String(query.searchTitle ?? '').trim(),
        problemIdSearch: parsePositiveInteger(query.searchProblemId)
      }
    },
    buildQuery(state){
      const nextQuery = {}
      const searchMode = normalizeSearchMode(state.searchMode)

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

  async function applySearchQuery(nextState, preferredProblemId){
    const didNavigate = await queryState.navigate(nextState)

    if (!didNavigate) {
      await reloadProblems(preferredProblemId)
    }
  }

  async function selectProblem(problemId){
    if (problemId === selectedProblemId.value) {
      return
    }

    await replaceProblemRoute(problemId, {
      push: true
    })
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
    selectedProblemId,
    searchMode: queryState.localState.searchMode,
    titleSearchInput: queryState.localState.titleSearchInput,
    problemIdSearchInput: queryState.localState.problemIdSearchInput,
    routeSearchMode,
    routeTitleSearch,
    routeProblemIdSearch,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    syncSearchControlsFromRoute,
    setSearchMode,
    handleProblemIdSearchInput,
    replaceProblemRoute,
    selectProblem,
    submitSearch,
    resetSearch
  }
}
