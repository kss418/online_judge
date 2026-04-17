import { computed } from 'vue'

import {
  normalizeSearchMode
} from '@/queryState/problemAdminSearch'
import { parsePositiveInteger } from '@/utils/parse'

export function useAdminProblemSearchControls({
  queryState,
  selectedProblemId,
  formatCount,
  reloadProblems,
  showErrorNotice,
  createSearchState
}){
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
  function syncSearchControlsFromRoute(){
    queryState.syncFromRoute()
  }

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

      void applySearchQuery(createSearchState('problem-id', {
        problemId: nextProblemId
      }), nextProblemId)
      return
    }

    void applySearchQuery(createSearchState('title', {
      title: queryState.localState.titleSearchInput.value
    }), selectedProblemId.value)
  }

  function resetSearch(){
    resetSearchControls()
    void applySearchQuery(createSearchState('title'), selectedProblemId.value)
  }

  return {
    routeSearchMode,
    routeTitleSearch,
    routeProblemIdSearch,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    syncSearchControlsFromRoute,
    setSearchMode,
    updateTitleSearchInput,
    updateProblemIdSearchInput,
    resetSearchControls,
    submitSearch,
    resetSearch
  }
}
