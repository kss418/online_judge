import { computed, ref } from 'vue'

import { parsePositiveInteger } from '@/composables/adminProblems/problemHelpers'

export function useProblemSearchQuery({
  route,
  router,
  formatCount,
  getSelectedProblemId,
  reloadProblems,
  showErrorNotice
}){
  const searchMode = ref('title')
  const titleSearchInput = ref('')
  const problemIdSearchInput = ref('')

  const preferredProblemIdFromRoute = computed(() => {
    const routeProblemId = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId
    const parsedValue = Number.parseInt(String(routeProblemId ?? ''), 10)

    return Number.isInteger(parsedValue) && parsedValue > 0
      ? parsedValue
      : 0
  })
  const routeSearchMode = computed(() => {
    const rawValue = Array.isArray(route.query.searchMode)
      ? route.query.searchMode[0]
      : route.query.searchMode

    return rawValue === 'problem-id' ? 'problem-id' : 'title'
  })
  const routeTitleSearch = computed(() => {
    const rawValue = Array.isArray(route.query.searchTitle)
      ? route.query.searchTitle[0]
      : route.query.searchTitle

    return String(rawValue ?? '').trim()
  })
  const routeProblemIdSearch = computed(() => {
    const rawValue = Array.isArray(route.query.searchProblemId)
      ? route.query.searchProblemId[0]
      : route.query.searchProblemId

    return parsePositiveInteger(rawValue)
  })
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

  function syncSearchControlsFromRoute(){
    searchMode.value = routeSearchMode.value
    titleSearchInput.value = routeTitleSearch.value
    problemIdSearchInput.value = routeProblemIdSearch.value != null
      ? String(routeProblemIdSearch.value)
      : ''
  }

  function setSearchMode(nextMode){
    searchMode.value = nextMode
  }

  function handleProblemIdSearchInput(event){
    const normalizedValue = String(event.target?.value ?? '').replace(/\D+/g, '')
    problemIdSearchInput.value = normalizedValue
  }

  function buildSearchQuery(mode, options = {}){
    const nextQuery = {}
    const selectedProblemIdFromRoute = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId

    if (selectedProblemIdFromRoute) {
      nextQuery.problemId = String(selectedProblemIdFromRoute)
    }

    if (mode === 'problem-id') {
      const nextProblemId = parsePositiveInteger(options.problemId)
      if (nextProblemId != null) {
        nextQuery.searchMode = 'problem-id'
        nextQuery.searchProblemId = String(nextProblemId)
      }

      return nextQuery
    }

    const nextTitle = String(options.title ?? '').trim()
    if (nextTitle) {
      nextQuery.searchMode = 'title'
      nextQuery.searchTitle = nextTitle
    }

    return nextQuery
  }

  function isSameSearchQuery(nextQuery){
    const currentQuery = buildSearchQuery(routeSearchMode.value, {
      title: routeTitleSearch.value,
      problemId: routeProblemIdSearch.value
    })

    return JSON.stringify(currentQuery) === JSON.stringify(nextQuery)
  }

  async function applySearchQuery(nextQuery, preferredProblemId){
    if (isSameSearchQuery(nextQuery)) {
      await reloadProblems(preferredProblemId)
      return
    }

    await router.replace({
      query: nextQuery
    })
  }

  function submitSearch(){
    if (searchMode.value === 'problem-id') {
      const nextProblemId = parsePositiveInteger(problemIdSearchInput.value)
      if (nextProblemId == null) {
        showErrorNotice('문제 번호를 입력하세요.')
        return
      }

      const nextQuery = buildSearchQuery('problem-id', {
        problemId: nextProblemId
      })
      void applySearchQuery(nextQuery, nextProblemId)
      return
    }

    const nextTitle = titleSearchInput.value.trim()
    const nextQuery = buildSearchQuery('title', {
      title: nextTitle
    })
    void applySearchQuery(nextQuery, getSelectedProblemId())
  }

  function resetSearch(){
    searchMode.value = 'title'
    titleSearchInput.value = ''
    problemIdSearchInput.value = ''
    void applySearchQuery(buildSearchQuery('title'), getSelectedProblemId())
  }

  return {
    searchMode,
    titleSearchInput,
    problemIdSearchInput,
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
    buildSearchQuery,
    submitSearch,
    resetSearch
  }
}
