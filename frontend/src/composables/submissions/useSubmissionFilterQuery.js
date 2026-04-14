import { computed, ref } from 'vue'

import { useRouteQueryState } from '@/composables/useRouteQueryState'
import {
  buildRouteQuery as buildSubmissionRouteQuery,
  normalizeProblemIdFilterInputValue,
  normalizeUserLoginIdFilterInputValue,
  parseRouteQuery as parseSubmissionRouteQuery
} from '@/queryState/submissionFilters'
import { parsePositiveInteger } from '@/utils/parse'

export function useSubmissionFilterQuery({ route, router, formatCount }){
  const fixedProblemId = computed(() => {
    const problemIdParam = Array.isArray(route.params.problemId)
      ? route.params.problemId[0]
      : route.params.problemId

    return parsePositiveInteger(problemIdParam)
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
    parseQuery: parseSubmissionRouteQuery,
    buildQuery(state){
      return buildSubmissionRouteQuery(state, {
        hasFixedProblemId: hasFixedProblemId.value,
        isMineScope: isMineScope.value,
        routeName: String(route.name ?? '')
      })
    },
    createLocalState(){
      return {
        selectedProblemIdFilter: ref(''),
        selectedUserLoginIdFilter: ref(''),
        selectedStatusFilter: ref(''),
        selectedLanguageFilter: ref('')
      }
    },
    syncLocalState(localState, state){
      localState.selectedProblemIdFilter.value = hasFixedProblemId.value
        ? ''
        : state.problemIdFilter
      localState.selectedUserLoginIdFilter.value = isMineScope.value
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

  const showUserLoginIdFilter = computed(() => !isMineScope.value)
  const appliedProblemIdFilter = computed(() => (
    hasFixedProblemId.value ? '' : queryState.routeState.value.problemIdFilter
  ))
  const appliedUserLoginIdFilter = computed(() => (
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

    const parsedProblemId = parsePositiveInteger(trimmedProblemId)

    return parsedProblemId != null ? String(parsedProblemId) : null
  })
  const normalizedSelectedUserLoginIdFilter = computed(() => {
    if (isMineScope.value) {
      return ''
    }

    const trimmedUserLoginId = normalizeUserLoginIdFilterInputValue(
      queryState.localState.selectedUserLoginIdFilter.value
    )
    if (!trimmedUserLoginId) {
      return ''
    }

    return trimmedUserLoginId
  })
  const hasInvalidProblemIdFilter = computed(() =>
    !hasFixedProblemId.value &&
    normalizedSelectedProblemIdFilter.value === null
  )
  const canApplyFilters = computed(() =>
    !hasInvalidProblemIdFilter.value &&
    (
      queryState.localState.selectedStatusFilter.value !== appliedStatusFilter.value ||
      queryState.localState.selectedLanguageFilter.value !== appliedLanguageFilter.value ||
      (
        !isMineScope.value &&
        normalizedSelectedUserLoginIdFilter.value !== appliedUserLoginIdFilter.value
      ) ||
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
      Boolean(normalizeUserLoginIdFilterInputValue(
        queryState.localState.selectedUserLoginIdFilter.value
      )) ||
      Boolean(appliedUserLoginIdFilter.value)
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

    return parsePositiveInteger(appliedProblemIdFilter.value)
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

  function buildSubmissionQueryState(problemId, userLoginId, status, language){
    return {
      scope: queryState.routeState.value.scope,
      problemIdFilter: hasFixedProblemId.value ? '' : String(problemId ?? ''),
      userLoginId: isMineScope.value ? '' : String(userLoginId ?? '').trim(),
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
      normalizedSelectedUserLoginIdFilter.value || '',
      queryState.localState.selectedStatusFilter.value,
      queryState.localState.selectedLanguageFilter.value
    ))
  }

  async function resetSubmissionFilters(){
    queryState.localState.selectedProblemIdFilter.value = ''
    queryState.localState.selectedUserLoginIdFilter.value = ''
    queryState.localState.selectedStatusFilter.value = ''
    queryState.localState.selectedLanguageFilter.value = ''

    if (!canResetFilters.value) {
      return
    }

    await queryState.navigate(buildSubmissionQueryState('', '', '', ''))
  }

  return {
    selectedProblemIdFilter: queryState.localState.selectedProblemIdFilter,
    selectedUserLoginIdFilter: queryState.localState.selectedUserLoginIdFilter,
    selectedStatusFilter: queryState.localState.selectedStatusFilter,
    selectedLanguageFilter: queryState.localState.selectedLanguageFilter,
    fixedProblemId,
    hasFixedProblemId,
    isMineScope,
    showUserLoginIdFilter,
    appliedProblemIdFilter,
    appliedUserLoginIdFilter,
    appliedStatusFilter,
    appliedLanguageFilter,
    hasAppliedStatusFilter,
    normalizedSelectedProblemIdFilter,
    normalizedSelectedUserLoginIdFilter,
    canApplyFilters,
    canResetFilters,
    numericProblemId,
    routeState: queryState.routeState,
    pageTitle,
    syncSelectedFiltersFromRoute,
    applySubmissionFilters,
    resetSubmissionFilters
  }
}
