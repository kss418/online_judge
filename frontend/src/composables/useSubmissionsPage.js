import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getSupportedLanguages } from '@/api/http'
import {
  getSubmissionDetail,
  getSubmissionHistory,
  getSubmissionList,
  getSubmissionSource,
  rejudgeSubmission
} from '@/api/submission'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'
import { buildPaginationItems } from '@/utils/pagination'
import { getSubmissionStatusLabel } from '@/utils/submissionStatus'

const listLimit = 50
const submissionPollingIntervalMs = 2000
const pollingSubmissionStatuses = new Set(['queued', 'judging'])
const submissionStatusOptions = [
  { value: '', label: '전체' },
  { value: 'accepted', label: '정답' },
  { value: 'wrong_answer', label: '오답' },
  { value: 'time_limit_exceeded', label: '시간 초과' },
  { value: 'memory_limit_exceeded', label: '메모리 초과' },
  { value: 'runtime_error', label: '런타임 에러' },
  { value: 'output_exceeded', label: '출력 초과' },
  { value: 'compile_error', label: '컴파일 에러' }
]

const finishedSubmissionStatuses = new Set([
  'accepted',
  'wrong_answer',
  'time_limit_exceeded',
  'memory_limit_exceeded',
  'runtime_error',
  'output_exceeded',
  'compile_error'
])

export function useSubmissionsPage(){
  const route = useRoute()
  const router = useRouter()
  const { authState, isAuthenticated, initializeAuth } = useAuth()
  const { showErrorNotice, showSuccessNotice } = useNotice()
  const countFormatter = new Intl.NumberFormat()

  const validSubmissionStatusFilterValues = new Set(
    submissionStatusOptions
      .map((option) => option.value)
      .filter(Boolean)
  )

  const isLoading = ref(true)
  const isLoadingLanguages = ref(true)
  const errorMessage = ref('')
  const actionMessage = ref('')
  const actionErrorMessage = ref('')
  const submissions = ref([])
  const hasLoadedOnce = ref(false)
  const currentPage = ref(1)
  const pageJumpInput = ref('')
  const totalSubmissionCount = ref(0)
  const historyDialogOpen = ref(false)
  const isLoadingHistory = ref(false)
  const historyErrorMessage = ref('')
  const submissionHistoryEntries = ref([])
  const activeHistorySubmissionId = ref(null)
  const sourceDialogOpen = ref(false)
  const isLoadingSource = ref(false)
  const sourceErrorMessage = ref('')
  const sourceDetail = ref(null)
  const activeSourceSubmissionId = ref(null)
  const copyState = ref('idle')
  const nowTimestamp = ref(Date.now())
  const rejudgingSubmissionIds = ref([])
  const isDocumentVisible = ref(typeof document === 'undefined' ? true : !document.hidden)
  const selectedProblemIdFilter = ref('')
  const selectedStatusFilter = ref('')
  const selectedLanguageFilter = ref('')
  const supportedSubmissionLanguages = ref([])

  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const canManageSubmissionRejudge = computed(() =>
    Number(authState.currentUser?.permission_level ?? 0) >= 1
  )
  const submissionLanguageFilterOptions = computed(() => {
    const options = [{ value: '', label: '전체' }]
    const seenLanguages = new Set([''])
    const candidateLanguages = [
      ...supportedSubmissionLanguages.value,
      appliedLanguageFilter.value,
      selectedLanguageFilter.value
    ]

    candidateLanguages.forEach((language) => {
      if (!language || seenLanguages.has(language)) {
        return
      }

      seenLanguages.add(language)
      options.push({
        value: language,
        label: language
      })
    })

    return options
  })
  const submissionCount = computed(() => submissions.value.length)
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
  const hasInvalidProblemIdFilter = computed(() =>
    !hasFixedProblemId.value && normalizedSelectedProblemIdFilter.value === null
  )
  const canApplyFilters = computed(() =>
    !hasInvalidProblemIdFilter.value &&
    (
      selectedStatusFilter.value !== appliedStatusFilter.value ||
      selectedLanguageFilter.value !== appliedLanguageFilter.value ||
      (!hasFixedProblemId.value &&
        normalizedSelectedProblemIdFilter.value !== appliedProblemIdFilter.value)
    )
  )
  const canResetFilters = computed(() =>
    Boolean(selectedStatusFilter.value) ||
    Boolean(selectedLanguageFilter.value) ||
    hasAppliedStatusFilter.value ||
    Boolean(appliedLanguageFilter.value) ||
    (!hasFixedProblemId.value && (
      Boolean(normalizeProblemIdFilterInputValue(selectedProblemIdFilter.value)) ||
      Boolean(appliedProblemIdFilter.value)
    ))
  )
  const pollingSubmissionIds = computed(() =>
    submissions.value
      .filter((submission) => pollingSubmissionStatuses.has(submission.status))
      .map((submission) => submission.submission_id)
  )
  const totalPages = computed(() =>
    Math.max(1, Math.ceil(totalSubmissionCount.value / listLimit))
  )
  const visibleRangeText = computed(() => {
    if (!totalSubmissionCount.value || !submissionCount.value) {
      return ''
    }

    const start = (currentPage.value - 1) * listLimit + 1
    const end = Math.min(currentPage.value * listLimit, totalSubmissionCount.value)
    return `${formatCount(start)}-${formatCount(end)} / ${formatCount(totalSubmissionCount.value)}`
  })
  const paginationItems = computed(() =>
    buildPaginationItems(currentPage.value, totalPages.value)
  )
  const numericProblemId = computed(() => {
    if (fixedProblemId.value !== null) {
      return fixedProblemId.value
    }

    return appliedProblemIdFilter.value
      ? Number.parseInt(appliedProblemIdFilter.value, 10)
      : null
  })
  const latestHistoryStatus = computed(() => {
    const latestHistoryEntry = submissionHistoryEntries.value[
      submissionHistoryEntries.value.length - 1
    ]

    return latestHistoryEntry?.to_status || ''
  })
  const shouldPollSubmissionHistory = computed(() =>
    historyDialogOpen.value &&
    !isLoadingHistory.value &&
    !historyErrorMessage.value &&
    Number.isInteger(activeHistorySubmissionId.value) &&
    isDocumentVisible.value &&
    Boolean(authState.token) &&
    canManageSubmissionRejudge.value &&
    pollingSubmissionStatuses.has(latestHistoryStatus.value)
  )
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
  const numericUserId = computed(() => {
    const userIdQuery = Array.isArray(route.query.userId)
      ? route.query.userId[0]
      : route.query.userId

    const parsedUserId = Number.parseInt(userIdQuery, 10)
    return Number.isInteger(parsedUserId) && parsedUserId > 0
      ? parsedUserId
      : null
  })
  const activeUserId = computed(() => {
    if (isMineScope.value) {
      const currentUserId = Number(authState.currentUser?.id)
      return Number.isInteger(currentUserId) && currentUserId > 0
        ? currentUserId
        : null
    }

    return numericUserId.value
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
  const copyButtonLabel = computed(() => {
    if (copyState.value === 'success') {
      return '복사됨'
    }

    if (copyState.value === 'error') {
      return '복사 실패'
    }

    return '복사'
  })

  let copyStateResetTimer = null
  let latestLoadRequestId = 0
  let latestHistoryLoadRequestId = 0
  let relativeTimeRefreshTimer = null
  let submissionPollingTimer = null
  let isPollingSubmissionDetails = false
  let historyPollingTimer = null
  let isPollingSubmissionHistory = false

  watch(actionMessage, (message) => {
    if (message) {
      showSuccessNotice(message)
    }
  })

  watch(actionErrorMessage, (message) => {
    if (message) {
      showErrorNotice(message, { duration: 5000 })
    }
  })

  watch(appliedProblemIdFilter, (problemId) => {
    if (!hasFixedProblemId.value) {
      selectedProblemIdFilter.value = problemId
    }
  }, {
    immediate: true
  })

  watch(appliedStatusFilter, (status) => {
    selectedStatusFilter.value = status
  }, {
    immediate: true
  })

  watch(appliedLanguageFilter, (language) => {
    selectedLanguageFilter.value = language
  }, {
    immediate: true
  })

  watch(authenticatedBearerToken, (nextToken, previousToken) => {
    if (!authState.initialized || nextToken === previousToken) {
      return
    }

    if (isMineScope.value) {
      resetPagination()
    }

    void loadSubmissions()
  })

  watch(totalPages, (pageCount) => {
    if (currentPage.value > pageCount) {
      currentPage.value = pageCount
    }
  })

  watch(
    [pollingSubmissionIds, isDocumentVisible, isLoading, errorMessage],
    () => {
      syncSubmissionPolling()
    },
    { immediate: true }
  )

  watch(
    [
      shouldPollSubmissionHistory,
      latestHistoryStatus,
      activeHistorySubmissionId
    ],
    () => {
      syncHistoryPolling()
    },
    { immediate: true }
  )

  watch(
    [
      () => route.name,
      numericProblemId,
      numericUserId,
      isMineScope,
      appliedLanguageFilter,
      appliedStatusFilter
    ],
    () => {
      if (!authState.initialized) {
        return
      }

      resetPagination()
      void loadSubmissions()
    }
  )

  function formatCount(value){
    return countFormatter.format(value)
  }

  function normalizeProblemIdFilterInputValue(value){
    if (value === null || typeof value === 'undefined') {
      return ''
    }

    return String(value).trim()
  }

  function normalizeSubmissionMetric(value){
    if (value === null || typeof value === 'undefined' || value === '') {
      return null
    }

    const parsedValue = Number(value)
    return Number.isFinite(parsedValue) ? parsedValue : null
  }

  function resetPagination(){
    currentPage.value = 1
    pageJumpInput.value = ''
    totalSubmissionCount.value = 0
  }

  async function loadSupportedSubmissionLanguages(){
    isLoadingLanguages.value = true

    try {
      const response = await getSupportedLanguages()
      const languages = Array.isArray(response.languages) ? response.languages : []

      supportedSubmissionLanguages.value = languages
        .map((language) => typeof language?.language === 'string' ? language.language.trim() : '')
        .filter((language, index, values) => language && values.indexOf(language) === index)
    } catch {
      supportedSubmissionLanguages.value = []
    } finally {
      isLoadingLanguages.value = false
    }
  }

  function makeSubmissionFilterQuery(problemId, status, language){
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
    if (isLoading.value || !canApplyFilters.value) {
      return
    }

    await router.replace({
      name: route.name,
      params: route.params,
      query: makeSubmissionFilterQuery(
        normalizedSelectedProblemIdFilter.value || '',
        selectedStatusFilter.value,
        selectedLanguageFilter.value
      )
    })
  }

  async function resetSubmissionFilters(){
    selectedProblemIdFilter.value = ''
    selectedStatusFilter.value = ''
    selectedLanguageFilter.value = ''

    if (isLoading.value || !canResetFilters.value) {
      return
    }

    await router.replace({
      name: route.name,
      params: route.params,
      query: makeSubmissionFilterQuery('', '', '')
    })
  }

  function normalizeSubmittedAt(value){
    if (typeof value !== 'string' || !value.trim()) {
      return {
        timestamp: null,
        label: ''
      }
    }

    const trimmedValue = value.trim()
    const matchedTimestamp = trimmedValue.match(
      /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
    )

    if (matchedTimestamp) {
      const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
        matchedTimestamp
      const normalizedFraction = fractionPart
        ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
        : ''
      const parsedTimestamp = Date.parse(
        `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
      )

      return {
        timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
        label: `${datePart} ${timePart}`
      }
    }

    const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
    return {
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: trimmedValue
    }
  }

  function formatRelativeSubmittedAt(timestamp){
    if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
      return '-'
    }

    const elapsedSeconds = Math.max(1, Math.floor((nowTimestamp.value - timestamp) / 1000))

    if (elapsedSeconds < 60) {
      return `${elapsedSeconds}초 전`
    }

    const elapsedMinutes = Math.floor(elapsedSeconds / 60)
    if (elapsedMinutes < 60) {
      return `${elapsedMinutes}분 전`
    }

    const elapsedHours = Math.floor(elapsedMinutes / 60)
    if (elapsedHours < 24) {
      return `${elapsedHours}시간 전`
    }

    const elapsedDays = Math.floor(elapsedHours / 24)
    if (elapsedDays < 30) {
      return `${elapsedDays}일 전`
    }

    const elapsedMonths = Math.floor(elapsedDays / 30)
    if (elapsedMonths < 12) {
      return `${elapsedMonths}달 전`
    }

    const elapsedYears = Math.floor(elapsedDays / 365)
    return `${elapsedYears}년 전`
  }

  function formatElapsedMs(value){
    if (typeof value !== 'number') {
      return '-'
    }

    return `${countFormatter.format(value)} ms`
  }

  function formatMemory(value){
    if (typeof value !== 'number') {
      return '-'
    }

    return `${countFormatter.format(value)} KB`
  }

  function normalizeSubmissionHistoryEntry(historyEntry, index){
    const normalizedCreatedAt = normalizeSubmittedAt(historyEntry?.created_at)
    const numericHistoryId = Number(historyEntry?.history_id)
    const fromStatus = typeof historyEntry?.from_status === 'string' && historyEntry.from_status
      ? historyEntry.from_status
      : null
    const toStatus = typeof historyEntry?.to_status === 'string' && historyEntry.to_status
      ? historyEntry.to_status
      : 'queued'
    const createdAtLabel = normalizedCreatedAt.label ||
      (typeof historyEntry?.created_at === 'string' && historyEntry.created_at.trim()
        ? historyEntry.created_at.trim()
        : '-')

    return {
      history_id: Number.isInteger(numericHistoryId) && numericHistoryId > 0
        ? numericHistoryId
        : null,
      history_key: Number.isInteger(numericHistoryId) && numericHistoryId > 0
        ? `history-${numericHistoryId}`
        : `history-fallback-${index}-${createdAtLabel}`,
      from_status: fromStatus,
      to_status: toStatus,
      reason: typeof historyEntry?.reason === 'string' ? historyEntry.reason.trim() : '',
      created_at_label: createdAtLabel
    }
  }

  function formatHistoryTransition(historyEntry){
    if (!historyEntry?.from_status) {
      return getSubmissionStatusLabel('queued')
    }

    return `${getSubmissionStatusLabel(historyEntry.from_status)} -> ${getSubmissionStatusLabel(historyEntry.to_status)}`
  }

  function canRejudgeSubmission(submission){
    if (!canManageSubmissionRejudge.value || !authState.token) {
      return false
    }

    return finishedSubmissionStatuses.has(submission.status)
  }

  function stopHistoryPolling(){
    if (historyPollingTimer) {
      clearTimeout(historyPollingTimer)
      historyPollingTimer = null
    }
  }

  function startHistoryPolling(){
    if (
      typeof window === 'undefined' ||
      historyPollingTimer ||
      isPollingSubmissionHistory ||
      !shouldPollSubmissionHistory.value
    ) {
      return
    }

    historyPollingTimer = window.setTimeout(() => {
      historyPollingTimer = null
      void pollSubmissionHistory()
    }, submissionPollingIntervalMs)
  }

  function syncHistoryPolling(){
    if (!shouldPollSubmissionHistory.value) {
      stopHistoryPolling()
      return
    }

    startHistoryPolling()
  }

  function isRejudgingSubmission(submissionId){
    return rejudgingSubmissionIds.value.includes(submissionId)
  }

  function addRejudgingSubmission(submissionId){
    if (isRejudgingSubmission(submissionId)) {
      return
    }

    rejudgingSubmissionIds.value = [...rejudgingSubmissionIds.value, submissionId]
  }

  function removeRejudgingSubmission(submissionId){
    rejudgingSubmissionIds.value = rejudgingSubmissionIds.value.filter(
      (queuedSubmissionId) => queuedSubmissionId !== submissionId
    )
  }

  function patchSubmission(submissionId, patch){
    submissions.value = submissions.value.map((submission) =>
      submission.submission_id === submissionId
        ? {
          ...submission,
          ...patch
        }
        : submission
    )
  }

  function shouldPollSubmissions(){
    return !isLoading.value &&
      !errorMessage.value &&
      isDocumentVisible.value &&
      !hasAppliedStatusFilter.value &&
      pollingSubmissionIds.value.length > 0
  }

  function stopSubmissionPolling(){
    if (submissionPollingTimer) {
      clearTimeout(submissionPollingTimer)
      submissionPollingTimer = null
    }
  }

  function startSubmissionPolling(){
    if (
      typeof window === 'undefined' ||
      submissionPollingTimer ||
      isPollingSubmissionDetails ||
      !shouldPollSubmissions()
    ) {
      return
    }

    submissionPollingTimer = window.setTimeout(() => {
      submissionPollingTimer = null
      void pollActiveSubmissions()
    }, submissionPollingIntervalMs)
  }

  function syncSubmissionPolling(){
    if (!shouldPollSubmissions()) {
      stopSubmissionPolling()
      return
    }

    startSubmissionPolling()
  }

  function updateSubmissionFromDetail(detail){
    const submissionId = Number(detail?.submission_id)

    if (!Number.isInteger(submissionId) || submissionId <= 0) {
      return
    }

    submissions.value = submissions.value.map((submission) =>
      submission.submission_id === submissionId
        ? {
          ...submission,
          status: typeof detail.status === 'string' && detail.status
            ? detail.status
            : submission.status,
          elapsed_ms: normalizeSubmissionMetric(detail.elapsed_ms),
          max_rss_kb: normalizeSubmissionMetric(detail.max_rss_kb)
        }
        : submission
    )
  }

  async function pollActiveSubmissions(){
    if (isPollingSubmissionDetails || !shouldPollSubmissions()) {
      return
    }

    isPollingSubmissionDetails = true
    const activeSubmissionIds = [...new Set(pollingSubmissionIds.value)]

    try {
      const results = await Promise.allSettled(
        activeSubmissionIds.map((submissionId) =>
          getSubmissionDetail(submissionId, {
            bearerToken: authenticatedBearerToken.value
          })
        )
      )

      results.forEach((result) => {
        if (result.status === 'fulfilled') {
          updateSubmissionFromDetail(result.value)
        }
      })
    } finally {
      isPollingSubmissionDetails = false
      syncSubmissionPolling()
    }
  }

  function handleDocumentVisibilityChange(){
    if (typeof document === 'undefined') {
      isDocumentVisible.value = true
    } else {
      isDocumentVisible.value = !document.hidden
    }

    syncSubmissionPolling()
  }

  function canViewSource(submission){
    if (!isAuthenticated.value || !authState.currentUser) {
      return false
    }

    if (Number(authState.currentUser.permission_level ?? 0) >= 1) {
      return true
    }

    return Number(submission.user_id) === Number(authState.currentUser.id)
  }

  function resetCopyState(){
    copyState.value = 'idle'

    if (copyStateResetTimer) {
      clearTimeout(copyStateResetTimer)
      copyStateResetTimer = null
    }
  }

  function scheduleCopyStateReset(){
    if (copyStateResetTimer) {
      clearTimeout(copyStateResetTimer)
    }

    copyStateResetTimer = window.setTimeout(() => {
      copyState.value = 'idle'
      copyStateResetTimer = null
    }, 1400)
  }

  function fallbackCopyText(text){
    if (typeof document === 'undefined') {
      throw new Error('clipboard unavailable')
    }

    const helperTextArea = document.createElement('textarea')
    helperTextArea.value = text
    helperTextArea.setAttribute('readonly', '')
    helperTextArea.style.position = 'fixed'
    helperTextArea.style.opacity = '0'
    helperTextArea.style.pointerEvents = 'none'

    document.body.appendChild(helperTextArea)
    helperTextArea.select()
    helperTextArea.setSelectionRange(0, helperTextArea.value.length)

    const copySucceeded = document.execCommand('copy')
    document.body.removeChild(helperTextArea)

    if (!copySucceeded) {
      throw new Error('clipboard unavailable')
    }
  }

  async function handleRejudgeSubmission(submission){
    if (!authState.token || !canRejudgeSubmission(submission) || isRejudgingSubmission(submission.submission_id)) {
      return
    }

    const submissionId = submission.submission_id
    addRejudgingSubmission(submissionId)
    actionMessage.value = ''
    actionErrorMessage.value = ''

    try {
      const response = await rejudgeSubmission(submissionId, authState.token)
      patchSubmission(submissionId, {
        status: response.status || 'queued',
        elapsed_ms: null,
        max_rss_kb: null
      })
      actionMessage.value = `제출 #${formatCount(submissionId)} 재채점을 요청했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '제출 재채점을 요청하지 못했습니다.'
    } finally {
      removeRejudgingSubmission(submissionId)
    }
  }

  function closeHistoryDialog(){
    stopHistoryPolling()
    latestHistoryLoadRequestId += 1
    historyDialogOpen.value = false
    isLoadingHistory.value = false
    historyErrorMessage.value = ''
    submissionHistoryEntries.value = []
    activeHistorySubmissionId.value = null
  }

  async function fetchSubmissionHistory(submissionId, options = {}){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    const { background = false } = options
    const requestId = ++latestHistoryLoadRequestId

    if (!background) {
      isLoadingHistory.value = true
      historyErrorMessage.value = ''
      submissionHistoryEntries.value = []
    }

    try {
      const response = await getSubmissionHistory(submissionId, authState.token)

      if (
        requestId !== latestHistoryLoadRequestId ||
        activeHistorySubmissionId.value !== submissionId
      ) {
        return
      }

      const normalizedHistories = Array.isArray(response.histories)
        ? response.histories.map((historyEntry, index) =>
          normalizeSubmissionHistoryEntry(historyEntry, index)
        )
        : []

      submissionHistoryEntries.value = normalizedHistories

      const latestHistoryEntry = normalizedHistories[normalizedHistories.length - 1]
      if (latestHistoryEntry) {
        patchSubmission(submissionId, {
          status: latestHistoryEntry.to_status
        })
      }
    } catch (error) {
      if (
        requestId !== latestHistoryLoadRequestId ||
        activeHistorySubmissionId.value !== submissionId
      ) {
        return
      }

      if (!background) {
        historyErrorMessage.value = error instanceof Error
          ? error.message
          : '채점 내역을 불러오지 못했습니다.'
        submissionHistoryEntries.value = []
      }
    } finally {
      if (
        !background &&
        requestId === latestHistoryLoadRequestId &&
        activeHistorySubmissionId.value === submissionId
      ) {
        isLoadingHistory.value = false
      }
    }
  }

  async function openHistoryDialog(submission){
    if (!authState.token || !canManageSubmissionRejudge.value) {
      return
    }

    historyDialogOpen.value = true
    isLoadingHistory.value = true
    historyErrorMessage.value = ''
    submissionHistoryEntries.value = []
    activeHistorySubmissionId.value = submission.submission_id

    await fetchSubmissionHistory(submission.submission_id)
  }

  async function pollSubmissionHistory(){
    if (
      isPollingSubmissionHistory ||
      !shouldPollSubmissionHistory.value ||
      !Number.isInteger(activeHistorySubmissionId.value)
    ) {
      return
    }

    isPollingSubmissionHistory = true

    try {
      await fetchSubmissionHistory(activeHistorySubmissionId.value, {
        background: true
      })
    } finally {
      isPollingSubmissionHistory = false
      syncHistoryPolling()
    }
  }

  function closeSourceDialog(){
    sourceDialogOpen.value = false
    isLoadingSource.value = false
    sourceErrorMessage.value = ''
    sourceDetail.value = null
    activeSourceSubmissionId.value = null
    resetCopyState()
  }

  async function openSourceDialog(submission){
    if (!canViewSource(submission) || !authState.token) {
      return
    }

    sourceDialogOpen.value = true
    isLoadingSource.value = true
    sourceErrorMessage.value = ''
    sourceDetail.value = null
    activeSourceSubmissionId.value = submission.submission_id

    try {
      const response = await getSubmissionSource(submission.submission_id, authState.token)
      sourceDetail.value = {
        submission_id: Number(response.submission_id),
        language: response.language || submission.language,
        source_code: response.source_code || '',
        compile_output: typeof response.compile_output === 'string' ? response.compile_output : '',
        judge_output: typeof response.judge_output === 'string' ? response.judge_output : ''
      }
    } catch (error) {
      sourceErrorMessage.value = error instanceof Error
        ? error.message
        : '소스 코드를 불러오지 못했습니다.'
    } finally {
      isLoadingSource.value = false
    }
  }

  async function copySourceCode(){
    if (!sourceDetail.value?.source_code) {
      return
    }

    try {
      if (typeof navigator !== 'undefined' && navigator.clipboard?.writeText) {
        await navigator.clipboard.writeText(sourceDetail.value.source_code)
      } else {
        fallbackCopyText(sourceDetail.value.source_code)
      }

      copyState.value = 'success'
    } catch {
      copyState.value = 'error'
    }

    scheduleCopyStateReset()
  }

  function stopRelativeTimeRefresh(){
    if (relativeTimeRefreshTimer) {
      clearInterval(relativeTimeRefreshTimer)
      relativeTimeRefreshTimer = null
    }
  }

  function startRelativeTimeRefresh(){
    stopRelativeTimeRefresh()
    nowTimestamp.value = Date.now()
    relativeTimeRefreshTimer = window.setInterval(() => {
      nowTimestamp.value = Date.now()
    }, 1000)
  }

  async function loadSubmissions(options = {}){
    const targetPageNumber = Number.isInteger(options.pageNumber) && options.pageNumber > 0
      ? options.pageNumber
      : currentPage.value
    const requestId = ++latestLoadRequestId
    isLoading.value = true
    errorMessage.value = ''
    rejudgingSubmissionIds.value = []

    if (isMineScope.value && !isAuthenticated.value) {
      submissions.value = []
      errorMessage.value = '내 제출을 보려면 로그인하세요.'
      if (requestId === latestLoadRequestId) {
        isLoading.value = false
      }
      hasLoadedOnce.value = true
      return
    }

    try {
      const response = await getSubmissionList({
        limit: listLimit,
        page: targetPageNumber,
        problemId: numericProblemId.value ?? undefined,
        userId: activeUserId.value ?? undefined,
        language: appliedLanguageFilter.value || undefined,
        status: appliedStatusFilter.value || undefined,
        bearerToken: authenticatedBearerToken.value
      })

      if (requestId !== latestLoadRequestId) {
        return
      }

      const normalizedSubmissions = Array.isArray(response.submissions)
        ? response.submissions
          .map((submission) => {
            const normalizedSubmittedAt = normalizeSubmittedAt(submission.created_at)

            return {
              ...submission,
              submission_id: Number(submission.submission_id),
              user_id: Number(submission.user_id),
              problem_id: Number(submission.problem_id),
              user_name: submission.user_name || `사용자 ${countFormatter.format(submission.user_id)}`,
              created_at: typeof submission.created_at === 'string' ? submission.created_at : '',
              created_at_timestamp: normalizedSubmittedAt.timestamp,
              created_at_label: normalizedSubmittedAt.label,
              elapsed_ms: typeof submission.elapsed_ms === 'number' ? submission.elapsed_ms : null,
              max_rss_kb: typeof submission.max_rss_kb === 'number' ? submission.max_rss_kb : null
            }
          })
          .sort((left, right) => right.submission_id - left.submission_id)
        : []

      const normalizedTotalSubmissionCount = Number(
        response.total_submission_count ?? response.submission_count ?? normalizedSubmissions.length
      )
      const nextTotalPages =
        normalizedTotalSubmissionCount > 0
          ? Math.max(1, Math.ceil(normalizedTotalSubmissionCount / listLimit))
          : 1

      if (normalizedTotalSubmissionCount > 0 && targetPageNumber > nextTotalPages) {
        currentPage.value = nextTotalPages
        void loadSubmissions({
          pageNumber: nextTotalPages
        })
        return
      }

      submissions.value = normalizedSubmissions
      totalSubmissionCount.value = normalizedTotalSubmissionCount
      currentPage.value = normalizedTotalSubmissionCount > 0 ? targetPageNumber : 1
      hasLoadedOnce.value = true
    } catch (error) {
      if (requestId !== latestLoadRequestId) {
        return
      }

      errorMessage.value = error instanceof Error
        ? error.message
        : '제출 목록을 불러오지 못했습니다.'
      submissions.value = []
      totalSubmissionCount.value = 0
      hasLoadedOnce.value = true
    } finally {
      if (requestId === latestLoadRequestId) {
        isLoading.value = false
      }
    }
  }

  function goToPage(pageNumber){
    if (isLoading.value) {
      return
    }

    const clampedPageNumber = Math.min(Math.max(pageNumber, 1), totalPages.value)
    if (clampedPageNumber === currentPage.value) {
      return
    }

    void loadSubmissions({
      pageNumber: clampedPageNumber
    })
  }

  function submitPageJump(){
    const parsedPage = Number.parseInt(pageJumpInput.value, 10)

    if (Number.isNaN(parsedPage)) {
      return
    }

    goToPage(parsedPage)
    pageJumpInput.value = ''
  }

  onMounted(async () => {
    startRelativeTimeRefresh()
    handleDocumentVisibilityChange()
    selectedProblemIdFilter.value = appliedProblemIdFilter.value
    selectedStatusFilter.value = appliedStatusFilter.value
    selectedLanguageFilter.value = appliedLanguageFilter.value

    if (typeof document !== 'undefined') {
      document.addEventListener('visibilitychange', handleDocumentVisibilityChange)
    }

    void loadSupportedSubmissionLanguages()

    if (!authState.initialized) {
      await initializeAuth()
    }

    if (!hasLoadedOnce.value) {
      void loadSubmissions()
    }
  })

  onUnmounted(() => {
    stopHistoryPolling()
    stopSubmissionPolling()
    stopRelativeTimeRefresh()
    resetCopyState()

    if (typeof document !== 'undefined') {
      document.removeEventListener('visibilitychange', handleDocumentVisibilityChange)
    }
  })

  return {
    authState,
    isAuthenticated,
    listLimit,
    submissionStatusOptions,
    isLoading,
    isLoadingLanguages,
    errorMessage,
    submissions,
    currentPage,
    pageJumpInput,
    totalSubmissionCount,
    historyDialogOpen,
    isLoadingHistory,
    historyErrorMessage,
    submissionHistoryEntries,
    activeHistorySubmissionId,
    sourceDialogOpen,
    isLoadingSource,
    sourceErrorMessage,
    sourceDetail,
    activeSourceSubmissionId,
    copyButtonLabel,
    selectedProblemIdFilter,
    selectedStatusFilter,
    selectedLanguageFilter,
    submissionLanguageFilterOptions,
    canManageSubmissionRejudge,
    hasFixedProblemId,
    canApplyFilters,
    canResetFilters,
    totalPages,
    visibleRangeText,
    paginationItems,
    numericProblemId,
    shouldPollSubmissionHistory,
    pageTitle,
    formatCount,
    formatRelativeSubmittedAt,
    formatElapsedMs,
    formatMemory,
    formatHistoryTransition,
    canViewSource,
    canRejudgeSubmission,
    isRejudgingSubmission,
    applySubmissionFilters,
    resetSubmissionFilters,
    openHistoryDialog,
    closeHistoryDialog,
    openSourceDialog,
    closeSourceDialog,
    copySourceCode,
    handleRejudgeSubmission,
    goToPage,
    submitPageJump
  }
}
