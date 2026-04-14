import { parsePositiveInteger } from '@/utils/parse'

const problemSortOptions = [
  {
    key: 'problem_id',
    label: '번호',
    defaultDirection: 'asc'
  },
  {
    key: 'accepted_count',
    label: '정답',
    defaultDirection: 'desc'
  },
  {
    key: 'acceptance_rate',
    label: '정답률',
    defaultDirection: 'desc'
  },
  {
    key: 'submission_count',
    label: '제출',
    defaultDirection: 'desc'
  }
]

const problemStateFilterOptions = [
  { value: '', label: '전체' },
  { value: 'solved', label: '해결' },
  { value: 'unsolved', label: '미해결' }
]

const validProblemSortKeys = new Set(problemSortOptions.map((option) => option.key))
const validProblemStateFilterValues = new Set(problemStateFilterOptions.map((option) => option.value))

export { problemSortOptions, problemStateFilterOptions }

export function normalizeProblemSortKey(rawValue){
  return typeof rawValue === 'string' && validProblemSortKeys.has(rawValue)
    ? rawValue
    : 'problem_id'
}

export function getDefaultProblemSortDirection(sortKey){
  return problemSortOptions.find((option) => option.key === sortKey)?.defaultDirection || 'asc'
}

export function normalizeProblemSortDirection(rawValue, sortKey){
  if (rawValue === 'asc' || rawValue === 'desc') {
    return rawValue
  }

  return getDefaultProblemSortDirection(sortKey)
}

export function normalizeProblemStateFilter(rawValue){
  return typeof rawValue === 'string' && validProblemStateFilterValues.has(rawValue)
    ? rawValue
    : ''
}

export function normalizeProblemPage(rawValue){
  return parsePositiveInteger(rawValue) ?? 1
}

export function parseRouteQuery(query, options = {}){
  const { includeStateFilter = false } = options
  const sortKey = normalizeProblemSortKey(query.sort)

  return {
    title: String(query.title ?? '').trim(),
    sortKey,
    sortDirection: normalizeProblemSortDirection(query.direction, sortKey),
    stateFilter: includeStateFilter
      ? normalizeProblemStateFilter(query.state)
      : '',
    page: normalizeProblemPage(query.page)
  }
}

export function buildRouteQuery(state, options = {}){
  const { includeStateFilter = false } = options
  const nextQuery = {}
  const title = String(state.title ?? '').trim()
  const sortKey = normalizeProblemSortKey(state.sortKey)
  const sortDirection = normalizeProblemSortDirection(state.sortDirection, sortKey)
  const stateFilter = includeStateFilter
    ? normalizeProblemStateFilter(state.stateFilter)
    : ''
  const page = Number(state.page)

  if (title) {
    nextQuery.title = title
  }

  if (sortKey !== 'problem_id' || sortDirection !== getDefaultProblemSortDirection('problem_id')) {
    nextQuery.sort = sortKey
  }

  if (sortDirection !== getDefaultProblemSortDirection(sortKey)) {
    nextQuery.direction = sortDirection
  }

  if (includeStateFilter && stateFilter) {
    nextQuery.state = stateFilter
  }

  if (Number.isInteger(page) && page > 1) {
    nextQuery.page = String(page)
  }

  return nextQuery
}

export function buildApiQuery(state, options = {}){
  const { includeStateFilter = false, pageSize = 50 } = options
  const parsedState = parseRouteQuery(state, { includeStateFilter })

  return {
    title: parsedState.title,
    state: includeStateFilter ? parsedState.stateFilter : '',
    sort: parsedState.sortKey,
    direction: parsedState.sortDirection,
    limit: pageSize,
    offset: (parsedState.page - 1) * pageSize
  }
}
