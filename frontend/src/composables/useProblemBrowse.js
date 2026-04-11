import {
  computed,
  onMounted,
  reactive,
  watch
} from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getProblemList } from '@/api/problem'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { useAuth } from '@/composables/useAuth'
import { useRouteQueryState } from '@/composables/useRouteQueryState'
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

function createInitialProblemBrowseState(){
  return {
    problems: [],
    totalProblemCount: 0
  }
}

export function useProblemBrowse(){
  const route = useRoute()
  const router = useRouter()
  const { authState, isAuthenticated, initializeAuth } = useAuth()

  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const showProblemStateFilters = computed(() =>
    authState.initialized && isAuthenticated.value
  )

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

  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      const sortKey = normalizeProblemSortKey(query.sort)

      return {
        title: String(query.title ?? '').trim(),
        sortKey,
        sortDirection: normalizeProblemSortDirection(query.direction, sortKey),
        stateFilter: showProblemStateFilters.value
          ? normalizeProblemStateFilter(query.state)
          : '',
        page: normalizeProblemPage(query.page)
      }
    },
    buildQuery(state){
      const nextQuery = {}
      const title = String(state.title ?? '').trim()
      const sortKey = normalizeProblemSortKey(state.sortKey)
      const sortDirection = normalizeProblemSortDirection(state.sortDirection, sortKey)
      const stateFilter = showProblemStateFilters.value
        ? normalizeProblemStateFilter(state.stateFilter)
        : ''
      const page = Number(state.page)

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
    },
    createLocalState(){
      return reactive({
        searchInput: '',
        pageJumpInput: ''
      })
    },
    syncLocalState(localState, state){
      localState.searchInput = state.title
    },
    buildLocation({ query }){
      return {
        name: 'problems',
        query
      }
    }
  })
  const problemListResource = useAsyncResource({
    initialData: createInitialProblemBrowseState,
    async load({ requestOffset, bearerToken, title, state, sort, direction }){
      const response = await getProblemList({
        title,
        state,
        sort,
        direction,
        limit: pageSize,
        offset: requestOffset,
        bearerToken
      })

      const problems = Array.isArray(response.problems)
        ? response.problems.map((problem) => ({
          ...problem,
          accepted_count: Number(problem.accepted_count ?? 0),
          submission_count: Number(problem.submission_count ?? 0)
        }))
        : []

      return {
        problems,
        totalProblemCount: Number(
          response.total_problem_count ?? response.problem_count ?? problems.length
        )
      }
    },
    getErrorMessage(error){
      return error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
    }
  })

  const isLoading = problemListResource.isLoading
  const errorMessage = problemListResource.errorMessage
  const hasLoadedOnce = problemListResource.hasLoadedOnce
  const appliedTitleFilter = computed(() => queryState.routeState.value.title)
  const hasAppliedTitleFilter = computed(() => Boolean(appliedTitleFilter.value))
  const appliedSortKey = computed(() => queryState.routeState.value.sortKey)
  const appliedSortDirection = computed(() => queryState.routeState.value.sortDirection)
  const appliedStateFilter = computed(() => queryState.routeState.value.stateFilter)
  const currentPage = computed(() => queryState.routeState.value.page)
  const problemCount = computed(() => problemListResource.data.value.totalProblemCount)
  const totalPages = computed(() =>
    Math.max(1, Math.ceil(problemCount.value / pageSize))
  )
  const pagedProblems = computed(() => problemListResource.data.value.problems)
  const visibleRangeText = computed(() => {
    if (!problemCount.value || !pagedProblems.value.length) {
      return ''
    }

    const start = (currentPage.value - 1) * pageSize + 1
    const end = start + pagedProblems.value.length - 1
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

  function buildProblemBrowseState(options = {}){
    return {
      title: options.title ?? appliedTitleFilter.value,
      sortKey: options.sortKey ?? appliedSortKey.value,
      sortDirection: options.sortDirection ?? appliedSortDirection.value,
      stateFilter: options.stateFilter ?? appliedStateFilter.value,
      page: Number(options.page ?? currentPage.value)
    }
  }

  async function replaceProblemBrowseQuery(options = {}){
    await queryState.navigate(buildProblemBrowseState(options))
  }

  async function loadProblems(){
    await problemListResource.run({
      requestOffset: (currentPage.value - 1) * pageSize,
      bearerToken: authenticatedBearerToken.value,
      title: appliedTitleFilter.value,
      state: appliedStateFilter.value,
      sort: appliedSortKey.value,
      direction: appliedSortDirection.value
    }, {
      resetDataOnError: true
    })
  }

  watch(appliedTitleFilter, () => {
    queryState.syncFromRoute()
  }, {
    immediate: true
  })

  watch(currentPage, () => {
    queryState.localState.pageJumpInput = ''
  })

  watch(totalPages, (pageCount) => {
    if (currentPage.value > pageCount) {
      void replaceProblemBrowseQuery({
        page: pageCount
      })
    }
  })

  watch(
    [
      appliedTitleFilter,
      appliedSortKey,
      appliedSortDirection,
      appliedStateFilter,
      currentPage,
      authenticatedBearerToken
    ],
    () => {
      if (!hasLoadedOnce.value) {
        return
      }

      void loadProblems()
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
      title: queryState.localState.searchInput.trim(),
      page: 1
    })
  }

  async function resetSearch(){
    queryState.localState.searchInput = ''

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
    const parsedPage = Number.parseInt(queryState.localState.pageJumpInput, 10)

    if (Number.isNaN(parsedPage)) {
      return
    }

    await goToPage(Math.min(Math.max(parsedPage, 1), totalPages.value))
    queryState.localState.pageJumpInput = ''
  }

  onMounted(async () => {
    if (!authState.initialized) {
      await initializeAuth()
    }

    if (!hasLoadedOnce.value) {
      void loadProblems()
    }
  })

  return {
    searchState: queryState.localState,
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
