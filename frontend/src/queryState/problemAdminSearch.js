import { parsePositiveInteger } from '@/utils/parse'

function normalizeSearchMode(value){
  return value === 'problem-id' ? 'problem-id' : 'title'
}

export { normalizeSearchMode }

export function parseRouteQuery(query){
  return {
    searchMode: normalizeSearchMode(query.searchMode),
    titleSearch: String(query.searchTitle ?? '').trim(),
    problemIdSearch: parsePositiveInteger(query.searchProblemId)
  }
}

export function buildRouteQuery(state){
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
}

export function buildApiQuery(state){
  const parsedState = parseRouteQuery(state)

  return {
    title: parsedState.searchMode === 'title' ? parsedState.titleSearch : '',
    problemId: parsedState.searchMode === 'problem-id'
      ? parsedState.problemIdSearch
      : null
  }
}
