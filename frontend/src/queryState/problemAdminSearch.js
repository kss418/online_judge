function normalizeSearchMode(value){
  return value === 'problem-id' ? 'problem-id' : 'title'
}

function parsePositiveInteger(value){
  const numericValue = Number.parseInt(String(value), 10)
  return Number.isInteger(numericValue) && numericValue > 0 ? numericValue : null
}

export { normalizeSearchMode, parsePositiveInteger }

export function parseRouteQuery(query, options = {}){
  const { includeSelectedProblemId = false } = options

  return {
    selectedProblemId: includeSelectedProblemId
      ? (parsePositiveInteger(query.problemId) ?? 0)
      : 0,
    searchMode: normalizeSearchMode(query.searchMode),
    titleSearch: String(query.searchTitle ?? '').trim(),
    problemIdSearch: parsePositiveInteger(query.searchProblemId)
  }
}

export function buildRouteQuery(state, options = {}){
  const { includeSelectedProblemId = false } = options
  const nextQuery = {}
  const searchMode = normalizeSearchMode(state.searchMode)
  const selectedProblemId = includeSelectedProblemId
    ? Number(state.selectedProblemId ?? 0)
    : 0

  if (includeSelectedProblemId && selectedProblemId > 0) {
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
