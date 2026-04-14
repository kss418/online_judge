import {
  computed,
  onMounted,
  reactive,
  watch
} from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getProblemList } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { useRouteQueryState } from '@/composables/useRouteQueryState'
import {
  buildApiQuery as buildProblemBrowseApiQuery,
  buildRouteQuery as buildProblemBrowseRouteQuery,
  getDefaultProblemSortDirection,
  parseRouteQuery as parseProblemBrowseRouteQuery,
  problemSortOptions,
  problemStateFilterOptions
} from '@/queryState/problemBrowse'
import { authStore } from '@/stores/auth/authStore'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'
import { buildPaginationItems } from '@/utils/pagination'
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
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore

  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const showProblemStateFilters = computed(() =>
    authState.initialized && isAuthenticated.value
  )

  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      return parseProblemBrowseRouteQuery(query, {
        includeStateFilter: showProblemStateFilters.value
      })
    },
    buildQuery(state){
      return buildProblemBrowseRouteQuery(state, {
        includeStateFilter: showProblemStateFilters.value
      })
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
    async load({ bearerToken, routeState }){
      const response = await getProblemList({
        ...buildProblemBrowseApiQuery(routeState, {
          includeStateFilter: showProblemStateFilters.value,
          pageSize
        }),
        bearerToken
      })

      return {
        problems: response.problems,
        totalProblemCount: response.total_problem_count || response.problem_count
      }
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 목록을 불러오지 못했습니다.'
      })
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
      bearerToken: authenticatedBearerToken.value,
      routeState: queryState.routeState.value
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
      : getDefaultProblemSortDirection(sortKey)

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
    const parsedPage = parsePositiveInteger(queryState.localState.pageJumpInput)
    if (parsedPage == null) {
      queryState.localState.pageJumpInput = ''
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
