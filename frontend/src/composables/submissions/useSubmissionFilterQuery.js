import { computed, ref } from 'vue'

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

  const selectedProblemIdFilter = ref('')
  const selectedUserIdFilter = ref('')
  const selectedStatusFilter = ref('')
  const selectedLanguageFilter = ref('')

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
  const showUserIdFilter = computed(() => !isMineScope.value)
  const appliedProblemIdFilter = computed(() => {
    if (hasFixedProblemId.value) {
      return ''
    }

    const problemIdQuery = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId
    const parsedProblemId = Number.parseInt(problemIdQuery, 10)

    return Number.isInteger(parsedProblemId) && parsedProblemId > 0
      ? String(parsedProblemId)
      : ''
  })
  const appliedUserIdFilter = computed(() => {
    if (isMineScope.value) {
      return ''
    }

    const userLoginIdQuery = Array.isArray(route.query.userLoginId)
      ? route.query.userLoginId[0]
      : route.query.userLoginId

    return typeof userLoginIdQuery === 'string'
      ? userLoginIdQuery.trim()
      : ''
  })
  const appliedStatusFilter = computed(() => {
    const routeStatus = Array.isArray(route.query.status)
      ? route.query.status[0]
      : route.query.status

    if (typeof routeStatus !== 'string') {
      return ''
    }

    return validSubmissionStatusFilterValues.has(routeStatus)
      ? routeStatus
      : ''
  })
  const appliedLanguageFilter = computed(() => {
    const routeLanguage = Array.isArray(route.query.language)
      ? route.query.language[0]
      : route.query.language

    return typeof routeLanguage === 'string'
      ? routeLanguage.trim()
      : ''
  })
  const hasAppliedStatusFilter = computed(() => Boolean(appliedStatusFilter.value))
  const normalizedSelectedProblemIdFilter = computed(() => {
    if (hasFixedProblemId.value) {
      return ''
    }

    const trimmedProblemId = normalizeProblemIdFilterInputValue(selectedProblemIdFilter.value)
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

    const trimmedUserLoginId = normalizeUserIdFilterInputValue(selectedUserIdFilter.value)
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
      selectedStatusFilter.value !== appliedStatusFilter.value ||
      selectedLanguageFilter.value !== appliedLanguageFilter.value ||
      (!isMineScope.value &&
        normalizedSelectedUserIdFilter.value !== appliedUserIdFilter.value) ||
      (!hasFixedProblemId.value &&
        normalizedSelectedProblemIdFilter.value !== appliedProblemIdFilter.value)
    )
  )
  const canResetFilters = computed(() =>
    Boolean(selectedStatusFilter.value) ||
    Boolean(selectedLanguageFilter.value) ||
    hasAppliedStatusFilter.value ||
    Boolean(appliedLanguageFilter.value) ||
    (!isMineScope.value && (
      Boolean(normalizeUserIdFilterInputValue(selectedUserIdFilter.value)) ||
      Boolean(appliedUserIdFilter.value)
    )) ||
    (!hasFixedProblemId.value && (
      Boolean(normalizeProblemIdFilterInputValue(selectedProblemIdFilter.value)) ||
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

  function syncSelectedFiltersFromRoute(){
    selectedProblemIdFilter.value = hasFixedProblemId.value ? '' : appliedProblemIdFilter.value
    selectedUserIdFilter.value = isMineScope.value ? '' : appliedUserIdFilter.value
    selectedStatusFilter.value = appliedStatusFilter.value
    selectedLanguageFilter.value = appliedLanguageFilter.value
  }

  function makeSubmissionFilterQuery(problemId, userId, status, language){
    const nextQuery = {
      ...route.query
    }

    if (!hasFixedProblemId.value) {
      if (problemId) {
        nextQuery.problemId = problemId
      } else {
        delete nextQuery.problemId
      }
    }

    if (!isMineScope.value) {
      if (userId) {
        nextQuery.userLoginId = userId
      } else {
        delete nextQuery.userLoginId
      }
    } else {
      delete nextQuery.userLoginId
    }

    delete nextQuery.userId

    if (status) {
      nextQuery.status = status
    } else {
      delete nextQuery.status
    }

    if (language) {
      nextQuery.language = language
    } else {
      delete nextQuery.language
    }

    return nextQuery
  }

  async function applySubmissionFilters(){
    if (!canApplyFilters.value) {
      return
    }

    await router.replace({
      name: route.name,
      params: route.params,
      query: makeSubmissionFilterQuery(
        normalizedSelectedProblemIdFilter.value || '',
        normalizedSelectedUserIdFilter.value || '',
        selectedStatusFilter.value,
        selectedLanguageFilter.value
      )
    })
  }

  async function resetSubmissionFilters(){
    selectedProblemIdFilter.value = ''
    selectedUserIdFilter.value = ''
    selectedStatusFilter.value = ''
    selectedLanguageFilter.value = ''

    if (!canResetFilters.value) {
      return
    }

    await router.replace({
      name: route.name,
      params: route.params,
      query: makeSubmissionFilterQuery('', '', '', '')
    })
  }

  return {
    selectedProblemIdFilter,
    selectedUserIdFilter,
    selectedStatusFilter,
    selectedLanguageFilter,
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
