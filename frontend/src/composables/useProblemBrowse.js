import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getProblemList } from '@/api/problem'
import { useAuth } from '@/composables/useAuth'
import { buildPaginationItems } from '@/utils/pagination'

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
const pageSize = 50

export function useProblemBrowse(){
  const route = useRoute()
  const router = useRouter()
  const { authState, isAuthenticated, initializeAuth } = useAuth()

  const isLoading = ref(true)
  const errorMessage = ref('')
  const problems = ref([])
  const hasLoadedOnce = ref(false)
  const searchState = reactive({
    searchInput: '',
    pageJumpInput: ''
  })

  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const showProblemStateFilters = computed(() =>
    authState.initialized && isAuthenticated.value
  )
  const appliedTitleFilter = computed(() => {
    const routeTitle = Array.isArray(route.query.title)
      ? route.query.title[0]
      : route.query.title

    return typeof routeTitle === 'string'
      ? routeTitle.trim()
      : ''
  })
  const hasAppliedTitleFilter = computed(() => Boolean(appliedTitleFilter.value))
  const appliedSortKey = computed(() => {
    const routeSort = Array.isArray(route.query.sort)
      ? route.query.sort[0]
      : route.query.sort

    return normalizeProblemSortKey(routeSort)
  })
  const appliedSortDirection = computed(() => {
    const routeDirection = Array.isArray(route.query.direction)
      ? route.query.direction[0]
      : route.query.direction

    return normalizeProblemSortDirection(routeDirection, appliedSortKey.value)
  })
  const appliedStateFilter = computed(() => {
    if (!showProblemStateFilters.value) {
      return ''
    }

    const routeState = Array.isArray(route.query.state)
      ? route.query.state[0]
      : route.query.state

    return normalizeProblemStateFilter(routeState)
  })
  const currentPage = computed(() => {
    const routePage = Array.isArray(route.query.page)
      ? route.query.page[0]
      : route.query.page

    return normalizeProblemPage(routePage)
  })
  const filteredProblems = computed(() => {
    const visibleProblems = problems.value.filter((problem) => {
      if (appliedStateFilter.value === 'solved') {
        return problem.user_problem_state === 'solved'
      }

      if (appliedStateFilter.value === 'unsolved') {
        return problem.user_problem_state !== 'solved'
      }

      return true
    })

    return [...visibleProblems].sort(compareProblems)
  })
  const problemCount = computed(() => filteredProblems.value.length)
  const totalPages = computed(() =>
    Math.max(1, Math.ceil(problemCount.value / pageSize))
  )
  const pagedProblems = computed(() => {
    const startIndex = (currentPage.value - 1) * pageSize
    return filteredProblems.value.slice(startIndex, startIndex + pageSize)
  })
  const visibleRangeText = computed(() => {
    if (!problemCount.value) {
      return ''
    }

    const start = (currentPage.value - 1) * pageSize + 1
    const end = Math.min(currentPage.value * pageSize, problemCount.value)
    return `${start}-${end} / ${problemCount.value}`
  })
  const emptyStateMessage = computed(() => {
    if (hasAppliedTitleFilter.value || appliedStateFilter.value) {
      return '조건에 맞는 문제가 없습니다.'
    }

    return '등록된 문제가 아직 없습니다.'
  })
  const paginationItems = computed(() =>
    buildPaginationItems(currentPage.value, totalPages.value)
  )

  let latestLoadRequestId = 0

  watch(appliedTitleFilter, (title) => {
    searchState.searchInput = title
  }, {
    immediate: true
  })

  watch(currentPage, () => {
    searchState.pageJumpInput = ''
  })

  watch(totalPages, (pageCount) => {
    if (currentPage.value > pageCount) {
      void replaceProblemBrowseQuery({
        page: pageCount
      })
    }
  })

  watch(
    [appliedTitleFilter, authenticatedBearerToken],
    ([nextTitle, nextToken], [previousTitle, previousToken]) => {
      if (!hasLoadedOnce.value) {
        return
      }

      if (nextTitle === previousTitle && nextToken === previousToken) {
        return
      }

      loadProblems()
    }
  )

  watch(showProblemStateFilters, (canShowFilters, couldShowFilters) => {
    if (canShowFilters || !couldShowFilters || !route.query.state) {
      return
    }

    void replaceProblemBrowseQuery({
      stateFilter: '',
      page: 1
    })
  })

  function normalizeProblemSortKey(rawValue){
    return typeof rawValue === 'string' && validProblemSortKeys.has(rawValue)
      ? rawValue
      : 'problem_id'
  }

  function getDefaultSortDirection(sortKey){
    return problemSortOptions.find((option) => option.key === sortKey)?.defaultDirection || 'asc'
  }

  function normalizeProblemSortDirection(rawValue, sortKey){
    if (rawValue === 'asc' || rawValue === 'desc') {
      return rawValue
    }

    return getDefaultSortDirection(sortKey)
  }

  function normalizeProblemStateFilter(rawValue){
    return typeof rawValue === 'string' && validProblemStateFilterValues.has(rawValue)
      ? rawValue
      : ''
  }

  function normalizeProblemPage(rawValue){
    const parsedPage = Number.parseInt(rawValue, 10)
    return Number.isInteger(parsedPage) && parsedPage > 0
      ? parsedPage
      : 1
  }

  function compareAcceptanceRate(left, right){
    if (left.submission_count <= 0 && right.submission_count <= 0) {
      return 0
    }

    if (left.submission_count <= 0) {
      return -1
    }

    if (right.submission_count <= 0) {
      return 1
    }

    const comparisonValue =
      (left.accepted_count * right.submission_count) -
      (right.accepted_count * left.submission_count)

    if (comparisonValue > 0) {
      return 1
    }

    if (comparisonValue < 0) {
      return -1
    }

    return 0
  }

  function compareProblems(left, right){
    if (appliedSortKey.value === 'problem_id') {
      return appliedSortDirection.value === 'asc'
        ? left.problem_id - right.problem_id
        : right.problem_id - left.problem_id
    }

    if (appliedSortKey.value === 'acceptance_rate') {
      const rateComparison = compareAcceptanceRate(left, right)

      if (rateComparison !== 0) {
        return appliedSortDirection.value === 'asc'
          ? rateComparison
          : -rateComparison
      }

      return left.problem_id - right.problem_id
    }

    if (appliedSortKey.value === 'accepted_count') {
      const acceptedCountDifference = left.accepted_count - right.accepted_count
      if (acceptedCountDifference !== 0) {
        return appliedSortDirection.value === 'asc'
          ? acceptedCountDifference
          : -acceptedCountDifference
      }

      return left.problem_id - right.problem_id
    }

    const submissionCountDifference = left.submission_count - right.submission_count
    if (submissionCountDifference !== 0) {
      return appliedSortDirection.value === 'asc'
        ? submissionCountDifference
        : -submissionCountDifference
    }

    return left.problem_id - right.problem_id
  }

  function buildProblemBrowseQuery(options = {}){
    const title = options.title ?? appliedTitleFilter.value
    const sortKey = options.sortKey ?? appliedSortKey.value
    const sortDirection = options.sortDirection ?? appliedSortDirection.value
    const stateFilter = options.stateFilter ?? appliedStateFilter.value
    const page = Number(options.page ?? currentPage.value)
    const nextQuery = {}

    if (title) {
      nextQuery.title = title
    }

    if (sortKey !== 'problem_id' || sortDirection !== getDefaultSortDirection('problem_id')) {
      nextQuery.sort = sortKey
    }

    if (sortDirection !== getDefaultSortDirection(sortKey)) {
      nextQuery.direction = sortDirection
    }

    if (showProblemStateFilters.value && stateFilter) {
      nextQuery.state = stateFilter
    }

    if (Number.isInteger(page) && page > 1) {
      nextQuery.page = String(page)
    }

    return nextQuery
  }

  function areQueryValuesEqual(leftValue, rightValue){
    if (Array.isArray(leftValue) || Array.isArray(rightValue)) {
      const leftValues = Array.isArray(leftValue) ? leftValue : [leftValue]
      const rightValues = Array.isArray(rightValue) ? rightValue : [rightValue]

      if (leftValues.length !== rightValues.length) {
        return false
      }

      return leftValues.every((value, index) => value === rightValues[index])
    }

    return leftValue === rightValue
  }

  function areQueriesEqual(leftQuery, rightQuery){
    const leftKeys = Object.keys(leftQuery).sort()
    const rightKeys = Object.keys(rightQuery).sort()

    if (leftKeys.length !== rightKeys.length) {
      return false
    }

    return leftKeys.every((key, index) =>
      key === rightKeys[index] && areQueryValuesEqual(leftQuery[key], rightQuery[key])
    )
  }

  async function replaceProblemBrowseQuery(options = {}){
    const nextQuery = buildProblemBrowseQuery(options)

    if (areQueriesEqual(route.query, nextQuery)) {
      return
    }

    await router.replace({
      name: 'problems',
      query: nextQuery
    })
  }

  async function loadProblems(){
    const requestId = ++latestLoadRequestId
    isLoading.value = true
    errorMessage.value = ''

    try {
      const response = await getProblemList({
        title: appliedTitleFilter.value,
        bearerToken: authenticatedBearerToken.value
      })

      if (requestId !== latestLoadRequestId) {
        return
      }

      problems.value = Array.isArray(response.problems)
        ? response.problems.map((problem) => ({
          ...problem,
          accepted_count: Number(problem.accepted_count ?? 0),
          submission_count: Number(problem.submission_count ?? 0)
        }))
        : []
      hasLoadedOnce.value = true
    } catch (error) {
      if (requestId !== latestLoadRequestId) {
        return
      }

      errorMessage.value = error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
      problems.value = []
      hasLoadedOnce.value = true
    } finally {
      if (requestId === latestLoadRequestId) {
        isLoading.value = false
      }
    }
  }

  async function cycleSort(sortKey){
    const nextDirection = appliedSortKey.value === sortKey
      ? (appliedSortDirection.value === 'asc' ? 'desc' : 'asc')
      : getDefaultSortDirection(sortKey)

    await replaceProblemBrowseQuery({
      sortKey,
      sortDirection: nextDirection,
      page: 1
    })
  }

  async function applyStateFilter(stateFilter){
    if (!showProblemStateFilters.value) {
      return
    }

    await replaceProblemBrowseQuery({
      stateFilter,
      page: 1
    })
  }

  async function submitSearch(){
    await replaceProblemBrowseQuery({
      title: searchState.searchInput.trim(),
      page: 1
    })
  }

  async function resetSearch(){
    searchState.searchInput = ''

    await replaceProblemBrowseQuery({
      title: '',
      page: 1
    })
  }

  async function refreshProblems(){
    await loadProblems()
  }

  async function goToPage(pageNumber){
    if (pageNumber < 1 || pageNumber > totalPages.value) {
      return
    }

    await replaceProblemBrowseQuery({
      page: pageNumber
    })
  }

  async function submitPageJump(){
    const parsedPage = Number.parseInt(searchState.pageJumpInput, 10)

    if (Number.isNaN(parsedPage)) {
      return
    }

    await goToPage(Math.min(Math.max(parsedPage, 1), totalPages.value))
    searchState.pageJumpInput = ''
  }

  onMounted(async () => {
    if (!authState.initialized) {
      await initializeAuth()
    }

    if (!hasLoadedOnce.value) {
      loadProblems()
    }
  })

  return {
    searchState,
    isLoading,
    errorMessage,
    problemSortOptions,
    problemStateFilterOptions,
    showProblemStateFilters,
    hasAppliedTitleFilter,
    appliedSortKey,
    appliedSortDirection,
    appliedStateFilter,
    problemCount,
    pagedProblems,
    visibleRangeText,
    emptyStateMessage,
    currentPage,
    totalPages,
    paginationItems,
    submitSearch,
    resetSearch,
    refreshProblems,
    cycleSort,
    applyStateFilter,
    goToPage,
    submitPageJump
  }
}
