import { computed, ref } from 'vue'

import { useRouteQueryState } from '@/composables/useRouteQueryState'
import {
  normalizeProblemIdFilterInputValue,
  normalizeUserIdFilterInputValue,
  submissionStatusOptions
} from '@/composables/submissions/submissionHelpers'

export function useSubmissionFilterQuery({ route, router, formatCount }){
  const validSubmissionStatusFilterValues = new Set(
    submissionStatusOptions
      .map((option) => option.value)
      .filter(Boolean)
  )

  const fixedProblemId = computed(() => {
    const problemIdParam = Array.isArray(route.params.problemId)
      ? route.params.problemId[0]
      : route.params.problemId
    const parsedProblemId = Number.parseInt(problemIdParam, 10)

    return Number.isInteger(parsedProblemId) && parsedProblemId > 0
      ? parsedProblemId
      : null
  })
  const hasFixedProblemId = computed(() => fixedProblemId.value !== null)
  const isMineScope = computed(() => {
    if (route.name === 'problem-my-submissions') {
      return true
    }

    if (route.name === 'problem-submissions') {
      return false
    }

    const scopeQuery = Array.isArray(route.query.scope)
      ? route.query.scope[0]
      : route.query.scope

    return scopeQuery === 'mine'
  })
  const queryState = useRouteQueryState({
    route,
    router,
    parseQuery(query){
      const parsedProblemId = Number.parseInt(query.problemId, 10)

      return {
        scope: query.scope === 'mine' ? 'mine' : '',
        problemIdFilter: Number.isInteger(parsedProblemId) && parsedProblemId > 0
          ? String(parsedProblemId)
          : '',
        userLoginId: typeof query.userLoginId === 'string'
          ? query.userLoginId.trim()
          : '',
        status: validSubmissionStatusFilterValues.has(query.status)
          ? query.status
          : '',
        language: typeof query.language === 'string'
          ? query.language.trim()
          : ''
      }
    },
    buildQuery(state){
      const nextQuery = {}

      if (
        route.name !== 'problem-my-submissions' &&
        route.name !== 'problem-submissions' &&
        state.scope === 'mine'
      ) {
        nextQuery.scope = 'mine'
      }

      if (!hasFixedProblemId.value && state.problemIdFilter) {
        nextQuery.problemId = String(state.problemIdFilter)
      }

      if (!isMineScope.value && state.userLoginId) {
        nextQuery.userLoginId = state.userLoginId
      }

      if (state.status) {
        nextQuery.status = state.status
      }

      if (state.language) {
        nextQuery.language = state.language
      }

      return nextQuery
    },
    createLocalState(){
      return {
        selectedProblemIdFilter: ref(''),
        selectedUserIdFilter: ref(''),
        selectedStatusFilter: ref(''),
        selectedLanguageFilter: ref('')
      }
    },
    syncLocalState(localState, state){
      localState.selectedProblemIdFilter.value = hasFixedProblemId.value
        ? ''
        : state.problemIdFilter
      localState.selectedUserIdFilter.value = isMineScope.value
        ? ''
        : state.userLoginId
      localState.selectedStatusFilter.value = state.status
      localState.selectedLanguageFilter.value = state.language
    },
    buildLocation({ query }){
      return {
        name: route.name,
        params: route.params,
        query
      }
    }
  })

  const showUserIdFilter = computed(() => !isMineScope.value)
  const appliedProblemIdFilter = computed(() => (
    hasFixedProblemId.value ? '' : queryState.routeState.value.problemIdFilter
  ))
  const appliedUserIdFilter = computed(() => (
    isMineScope.value ? '' : queryState.routeState.value.userLoginId
  ))
  const appliedStatusFilter = computed(() => queryState.routeState.value.status)
  const appliedLanguageFilter = computed(() => queryState.routeState.value.language)
  const hasAppliedStatusFilter = computed(() => Boolean(appliedStatusFilter.value))
  const normalizedSelectedProblemIdFilter = computed(() => {
    if (hasFixedProblemId.value) {
      return ''
    }

    const trimmedProblemId = normalizeProblemIdFilterInputValue(
      queryState.localState.selectedProblemIdFilter.value
    )
    if (!trimmedProblemId) {
      return ''
    }

    const parsedProblemId = Number.parseInt(trimmedProblemId, 10)
    return Number.isInteger(parsedProblemId) && parsedProblemId > 0
      ? String(parsedProblemId)
      : null
  })
  const normalizedSelectedUserIdFilter = computed(() => {
    if (isMineScope.value) {
      return ''
    }

    const trimmedUserLoginId = normalizeUserIdFilterInputValue(
      queryState.localState.selectedUserIdFilter.value
    )
    if (!trimmedUserLoginId) {
      return ''
    }

    return trimmedUserLoginId
  })
  const hasInvalidProblemIdFilter = computed(() =>
    !hasFixedProblemId.value && normalizedSelectedProblemIdFilter.value === null
  )
  const hasInvalidUserIdFilter = computed(() => false)
  const canApplyFilters = computed(() =>
    !hasInvalidProblemIdFilter.value &&
    !hasInvalidUserIdFilter.value &&
    (
      queryState.localState.selectedStatusFilter.value !== appliedStatusFilter.value ||
      queryState.localState.selectedLanguageFilter.value !== appliedLanguageFilter.value ||
      (!isMineScope.value &&
        normalizedSelectedUserIdFilter.value !== appliedUserIdFilter.value) ||
      (!hasFixedProblemId.value &&
        normalizedSelectedProblemIdFilter.value !== appliedProblemIdFilter.value)
    )
  )
  const canResetFilters = computed(() =>
    Boolean(queryState.localState.selectedStatusFilter.value) ||
    Boolean(queryState.localState.selectedLanguageFilter.value) ||
    hasAppliedStatusFilter.value ||
    Boolean(appliedLanguageFilter.value) ||
    (!isMineScope.value && (
      Boolean(normalizeUserIdFilterInputValue(queryState.localState.selectedUserIdFilter.value)) ||
      Boolean(appliedUserIdFilter.value)
    )) ||
    (!hasFixedProblemId.value && (
      Boolean(normalizeProblemIdFilterInputValue(queryState.localState.selectedProblemIdFilter.value)) ||
      Boolean(appliedProblemIdFilter.value)
    ))
  )
  const numericProblemId = computed(() => {
    if (fixedProblemId.value !== null) {
      return fixedProblemId.value
    }

    return appliedProblemIdFilter.value
      ? Number.parseInt(appliedProblemIdFilter.value, 10)
      : null
  })
  const activeUserId = computed(() => {
    if (isMineScope.value) {
      return null
    }

    return null
  })
  const activeUserLoginId = computed(() => {
    if (isMineScope.value) {
      return ''
    }

    return appliedUserIdFilter.value
  })
  const pageTitle = computed(() =>
    isMineScope.value && numericProblemId.value
      ? `문제 #${formatCount(numericProblemId.value)} 내 제출`
      : isMineScope.value
        ? '내 제출'
        : numericProblemId.value
          ? `문제 #${formatCount(numericProblemId.value)} 제출 목록`
          : '제출 목록'
  )

  function buildSubmissionQueryState(problemId, userId, status, language){
    return {
      scope: queryState.routeState.value.scope,
      problemIdFilter: hasFixedProblemId.value ? '' : String(problemId ?? ''),
      userLoginId: isMineScope.value ? '' : String(userId ?? '').trim(),
      status: String(status ?? ''),
      language: String(language ?? '')
    }
  }

  function syncSelectedFiltersFromRoute(){
    queryState.syncFromRoute()
  }

  async function applySubmissionFilters(){
    if (!canApplyFilters.value) {
      return
    }

    await queryState.navigate(buildSubmissionQueryState(
      normalizedSelectedProblemIdFilter.value || '',
      normalizedSelectedUserIdFilter.value || '',
      queryState.localState.selectedStatusFilter.value,
      queryState.localState.selectedLanguageFilter.value
    ))
  }

  async function resetSubmissionFilters(){
    queryState.localState.selectedProblemIdFilter.value = ''
    queryState.localState.selectedUserIdFilter.value = ''
    queryState.localState.selectedStatusFilter.value = ''
    queryState.localState.selectedLanguageFilter.value = ''

    if (!canResetFilters.value) {
      return
    }

    await queryState.navigate(buildSubmissionQueryState('', '', '', ''))
  }

  return {
    selectedProblemIdFilter: queryState.localState.selectedProblemIdFilter,
    selectedUserIdFilter: queryState.localState.selectedUserIdFilter,
    selectedStatusFilter: queryState.localState.selectedStatusFilter,
    selectedLanguageFilter: queryState.localState.selectedLanguageFilter,
    fixedProblemId,
    hasFixedProblemId,
    isMineScope,
    showUserIdFilter,
    appliedProblemIdFilter,
    appliedUserIdFilter,
    appliedStatusFilter,
    appliedLanguageFilter,
    hasAppliedStatusFilter,
    normalizedSelectedProblemIdFilter,
    normalizedSelectedUserIdFilter,
    canApplyFilters,
    canResetFilters,
    numericProblemId,
    activeUserId,
    activeUserLoginId,
    pageTitle,
    syncSelectedFiltersFromRoute,
    applySubmissionFilters,
    resetSubmissionFilters
  }
}
