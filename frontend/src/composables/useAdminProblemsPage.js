import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import {
  createProblem,
  createProblemSample,
  deleteProblem,
  deleteProblemSample,
  getProblemDetail,
  getProblemList,
  rejudgeProblem,
  uploadProblemTestcaseZip,
  updateProblemLimits,
  updateProblemSample,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problem'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'

export function useAdminProblemsPage(){
  const { authState, isAuthenticated, initializeAuth } = useAuth()
  const { showErrorNotice, showSuccessNotice } = useNotice()
  const countFormatter = new Intl.NumberFormat()
  const route = useRoute()
  const router = useRouter()

  const isLoadingProblems = ref(true)
  const isLoadingDetail = ref(false)
  const listErrorMessage = ref('')
  const detailErrorMessage = ref('')
  const actionErrorMessage = ref('')
  const actionMessage = ref('')
  const searchMode = ref('title')
  const titleSearchInput = ref('')
  const problemIdSearchInput = ref('')
  const newProblemTitle = ref('')
  const problems = ref([])
  const selectedProblemId = ref(0)
  const selectedProblemDetail = ref(null)
  const titleDraft = ref('')
  const timeLimitDraft = ref('')
  const memoryLimitDraft = ref('')
  const descriptionDraft = ref('')
  const inputFormatDraft = ref('')
  const outputFormatDraft = ref('')
  const noteDraft = ref('')
  const sampleDrafts = ref([])
  const testcaseZipFile = ref(null)
  const testcaseZipInputKey = ref(0)
  const busySection = ref('')
  const rejudgeDialogOpen = ref(false)
  const rejudgeConfirmProblemId = ref('')
  const rejudgeConfirmTitle = ref('')
  const deleteDialogOpen = ref(false)
  const deleteConfirmProblemId = ref('')
  const deleteConfirmTitle = ref('')

  let latestProblemListRequestId = 0
  let latestProblemDetailRequestId = 0

  const canManageProblems = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
  const problemCount = computed(() => problems.value.length)
  const canCreateProblem = computed(() =>
    Boolean(authState.token) &&
    canManageProblems.value &&
    !busySection.value &&
    Boolean(newProblemTitle.value.trim())
  )
  const isCreatingProblem = computed(() => busySection.value === 'create')
  const isSavingTitle = computed(() => busySection.value === 'title')
  const isSavingLimits = computed(() => busySection.value === 'limits')
  const isSavingStatement = computed(() => busySection.value === 'statement')
  const isCreatingSample = computed(() => busySection.value === 'sample:create')
  const isDeletingLastSample = computed(() => busySection.value === 'sample:delete-last')
  const isUploadingTestcaseZip = computed(() => busySection.value === 'testcase:upload')
  const isRejudgingProblem = computed(() => busySection.value === 'rejudge')
  const isDeletingProblem = computed(() => busySection.value === 'delete')
  const selectedTestcaseZipName = computed(() => testcaseZipFile.value?.name || '')
  const canCreateSample = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canUploadTestcaseZip = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )
  const canDeleteLastSample = computed(() =>
    Boolean(selectedProblemDetail.value?.samples.length) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canSaveTitle = computed(() => {
    if (!selectedProblemDetail.value || !authState.token || busySection.value) {
      return false
    }

    const nextTitle = titleDraft.value.trim()
    return Boolean(nextTitle) && nextTitle !== selectedProblemDetail.value.title
  })
  const canSaveLimits = computed(() => {
    if (!selectedProblemDetail.value || !authState.token || busySection.value) {
      return false
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return false
    }

    return (
      nextTimeLimit !== selectedProblemDetail.value.limits.time_limit_ms ||
      nextMemoryLimit !== selectedProblemDetail.value.limits.memory_limit_mb
    )
  })
  const canSaveStatement = computed(() => {
    if (!selectedProblemDetail.value || !authState.token || busySection.value) {
      return false
    }

    if (
      !descriptionDraft.value.trim() ||
      !inputFormatDraft.value.trim() ||
      !outputFormatDraft.value.trim()
    ) {
      return false
    }

    return (
      descriptionDraft.value !== selectedProblemDetail.value.statement.description ||
      inputFormatDraft.value !== selectedProblemDetail.value.statement.input_format ||
      outputFormatDraft.value !== selectedProblemDetail.value.statement.output_format ||
      noteDraft.value !== selectedProblemDetail.value.statement.note
    )
  })
  const canDeleteSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !deleteDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      deleteConfirmProblemId.value.trim() === expectedProblemId &&
      deleteConfirmTitle.value === selectedProblemDetail.value.title
    )
  })
  const canRejudgeSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !rejudgeDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      rejudgeConfirmProblemId.value.trim() === expectedProblemId &&
      rejudgeConfirmTitle.value === selectedProblemDetail.value.title
    )
  })

  const preferredProblemIdFromRoute = computed(() => {
    const routeProblemId = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId
    const parsedValue = Number.parseInt(String(routeProblemId ?? ''), 10)

    return Number.isInteger(parsedValue) && parsedValue > 0
      ? parsedValue
      : 0
  })
  const routeSearchMode = computed(() => {
    const rawValue = Array.isArray(route.query.searchMode)
      ? route.query.searchMode[0]
      : route.query.searchMode

    return rawValue === 'problem-id' ? 'problem-id' : 'title'
  })
  const routeTitleSearch = computed(() => {
    const rawValue = Array.isArray(route.query.searchTitle)
      ? route.query.searchTitle[0]
      : route.query.searchTitle

    return String(rawValue ?? '').trim()
  })
  const routeProblemIdSearch = computed(() => {
    const rawValue = Array.isArray(route.query.searchProblemId)
      ? route.query.searchProblemId[0]
      : route.query.searchProblemId

    return parsePositiveInteger(rawValue)
  })
  const hasAppliedSearch = computed(() => {
    if (routeSearchMode.value === 'problem-id') {
      return routeProblemIdSearch.value != null
    }

    return Boolean(routeTitleSearch.value)
  })
  const problemListCaption = computed(() => {
    if (routeSearchMode.value === 'problem-id' && routeProblemIdSearch.value != null) {
      return `문제 #${formatCount(routeProblemIdSearch.value)} 검색 결과`
    }

    if (routeTitleSearch.value) {
      return `"${routeTitleSearch.value}" 검색 결과`
    }

    return '전체 문제'
  })
  const emptyProblemListMessage = computed(() => {
    if (routeSearchMode.value === 'problem-id' && routeProblemIdSearch.value != null) {
      return `문제 #${formatCount(routeProblemIdSearch.value)}를 찾지 못했습니다.`
    }

    if (routeTitleSearch.value) {
      return '검색 조건에 맞는 문제가 없습니다.'
    }

    return '등록된 문제가 아직 없습니다.'
  })
  const toolbarStatusLabel = computed(() =>
    isLoadingProblems.value ? 'Loading' : `${problemCount.value} Problems`
  )
  const toolbarStatusTone = computed(() =>
    listErrorMessage.value ? 'danger' : 'success'
  )

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

  watch(
    () => [authState.initialized, authState.token, authState.currentUser?.permission_level],
    () => {
      if (!authState.initialized) {
        isLoadingProblems.value = true
        return
      }

      if (!isAuthenticated.value || !canManageProblems.value) {
        resetPageState()
        isLoadingProblems.value = false
        return
      }

      syncSearchControlsFromRoute()
      void loadProblems({
        preferredProblemId: preferredProblemIdFromRoute.value || selectedProblemId.value
      })
    },
    { immediate: true }
  )

  watch(preferredProblemIdFromRoute, (problemId) => {
    if (!authState.initialized || !isAuthenticated.value || !canManageProblems.value || !problemId) {
      return
    }

    void loadProblems({ preferredProblemId: problemId })
  })

  watch(
    () => [routeSearchMode.value, routeTitleSearch.value, routeProblemIdSearch.value],
    () => {
      if (!authState.initialized || !isAuthenticated.value || !canManageProblems.value) {
        return
      }

      syncSearchControlsFromRoute()
      void loadProblems({
        preferredProblemId:
          routeSearchMode.value === 'problem-id'
            ? routeProblemIdSearch.value || selectedProblemId.value
            : selectedProblemId.value
      })
    }
  )

  function resetPageState(){
    searchMode.value = 'title'
    titleSearchInput.value = ''
    problemIdSearchInput.value = ''
    newProblemTitle.value = ''
    problems.value = []
    selectedProblemId.value = 0
    selectedProblemDetail.value = null
    listErrorMessage.value = ''
    detailErrorMessage.value = ''
    actionErrorMessage.value = ''
    actionMessage.value = ''
    busySection.value = ''
    closeRejudgeDialog(true)
    closeDeleteDialog(true)
    resetEditorDrafts()
  }

  function syncSearchControlsFromRoute(){
    searchMode.value = routeSearchMode.value
    titleSearchInput.value = routeTitleSearch.value
    problemIdSearchInput.value = routeProblemIdSearch.value != null
      ? String(routeProblemIdSearch.value)
      : ''
  }

  function setSearchMode(nextMode){
    searchMode.value = nextMode
  }

  function handleProblemIdSearchInput(event){
    const normalizedValue = String(event.target?.value ?? '').replace(/\D+/g, '')
    problemIdSearchInput.value = normalizedValue
  }

  function buildSearchQuery(mode, options = {}){
    const nextQuery = {}
    const selectedProblemIdFromRoute = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId

    if (selectedProblemIdFromRoute) {
      nextQuery.problemId = String(selectedProblemIdFromRoute)
    }

    if (mode === 'problem-id') {
      const nextProblemId = parsePositiveInteger(options.problemId)
      if (nextProblemId != null) {
        nextQuery.searchMode = 'problem-id'
        nextQuery.searchProblemId = String(nextProblemId)
      }

      return nextQuery
    }

    const nextTitle = String(options.title ?? '').trim()
    if (nextTitle) {
      nextQuery.searchMode = 'title'
      nextQuery.searchTitle = nextTitle
    }

    return nextQuery
  }

  function isSameSearchQuery(nextQuery){
    const currentQuery = buildSearchQuery(routeSearchMode.value, {
      title: routeTitleSearch.value,
      problemId: routeProblemIdSearch.value
    })

    return JSON.stringify(currentQuery) === JSON.stringify(nextQuery)
  }

  function resetEditorDrafts(){
    titleDraft.value = ''
    timeLimitDraft.value = ''
    memoryLimitDraft.value = ''
    descriptionDraft.value = ''
    inputFormatDraft.value = ''
    outputFormatDraft.value = ''
    noteDraft.value = ''
    sampleDrafts.value = []
    testcaseZipFile.value = null
    testcaseZipInputKey.value += 1
  }

  function formatCount(value){
    return countFormatter.format(Number(value ?? 0))
  }

  function formatProblemLimit(value, unit){
    const numericValue = Number(value)

    if (!Number.isFinite(numericValue) || numericValue <= 0) {
      return unit === 'ms' ? '시간 확인 중' : '메모리 확인 중'
    }

    return `${formatCount(numericValue)} ${unit}`
  }

  function normalizeProblemDetail(response){
    return {
      problem_id: Number(response.problem_id ?? 0),
      title: response.title ?? '',
      version: Number(response.version ?? 0),
      limits: {
        time_limit_ms: Number(response.limits?.time_limit_ms ?? 0),
        memory_limit_mb: Number(response.limits?.memory_limit_mb ?? 0)
      },
      statement: {
        description: response.statement?.description ?? '',
        input_format: response.statement?.input_format ?? '',
        output_format: response.statement?.output_format ?? '',
        note: response.statement?.note ?? ''
      },
      samples: normalizeProblemSamples(response.samples)
    }
  }

  function normalizeProblemSamples(samples){
    if (!Array.isArray(samples)) {
      return []
    }

    return samples
      .map((sample) => ({
        sample_order: Number(sample.sample_order ?? 0),
        sample_input: sample.sample_input ?? '',
        sample_output: sample.sample_output ?? ''
      }))
      .filter((sample) => sample.sample_order > 0)
      .sort((leftSample, rightSample) => leftSample.sample_order - rightSample.sample_order)
  }

  function makeSampleDraft(sample){
    return {
      sample_order: sample.sample_order,
      sample_input: sample.sample_input,
      sample_output: sample.sample_output
    }
  }

  function assignEditorDrafts(problemDetail){
    titleDraft.value = problemDetail.title
    timeLimitDraft.value = String(problemDetail.limits.time_limit_ms || '')
    memoryLimitDraft.value = String(problemDetail.limits.memory_limit_mb || '')
    descriptionDraft.value = problemDetail.statement.description
    inputFormatDraft.value = problemDetail.statement.input_format
    outputFormatDraft.value = problemDetail.statement.output_format
    noteDraft.value = problemDetail.statement.note
    sampleDrafts.value = problemDetail.samples.map(makeSampleDraft)
  }

  function mergeProblemSummary(problemId, patch){
    problems.value = problems.value.map((problem) =>
      problem.problem_id === problemId
        ? {
          ...problem,
          ...patch
        }
        : problem
    )
  }

  function getSelectedProblemSample(sampleOrder){
    return selectedProblemDetail.value?.samples.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function getSampleDraft(sampleOrder){
    return sampleDrafts.value.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function isSavingSample(sampleOrder){
    return busySection.value === makeSampleBusyKey(sampleOrder)
  }

  function makeSampleBusyKey(sampleOrder){
    return `sample:${sampleOrder}`
  }

  function canSaveSample(sampleOrder){
    if (!selectedProblemDetail.value || !authState.token || busySection.value) {
      return false
    }

    const selectedSample = getSelectedProblemSample(sampleOrder)
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!selectedSample || !sampleDraft) {
      return false
    }

    return (
      sampleDraft.sample_input !== selectedSample.sample_input ||
      sampleDraft.sample_output !== selectedSample.sample_output
    )
  }

  function isLastSample(sampleOrder){
    const samples = selectedProblemDetail.value?.samples || []
    if (!samples.length) {
      return false
    }

    return samples[samples.length - 1].sample_order === sampleOrder
  }

  function setSelectedProblemSamples(samples){
    if (!selectedProblemDetail.value) {
      return
    }

    const normalizedSamples = normalizeProblemSamples(samples)
    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      samples: normalizedSamples
    }
    sampleDrafts.value = normalizedSamples.map(makeSampleDraft)
  }

  function applySelectedProblemVersion(problemId, version){
    const normalizedVersion = Number(version)
    if (!Number.isInteger(normalizedVersion) || normalizedVersion <= 0) {
      return
    }

    const selectedProblem = selectedProblemDetail.value
    if (selectedProblem && selectedProblem.problem_id === problemId) {
      selectedProblemDetail.value = {
        ...selectedProblem,
        version: normalizedVersion
      }
    }

    mergeProblemSummary(problemId, {
      version: normalizedVersion
    })
  }

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return
    }

    const requestId = ++latestProblemListRequestId
    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
    isLoadingProblems.value = true
    listErrorMessage.value = ''
    actionErrorMessage.value = ''

    try {
      const activeTitleSearch = routeSearchMode.value === 'title'
        ? routeTitleSearch.value
        : ''
      const activeProblemIdSearch = routeSearchMode.value === 'problem-id'
        ? routeProblemIdSearch.value
        : null
      const response = await getProblemList({
        title: activeTitleSearch,
        bearerToken: authState.token
      })

      if (requestId !== latestProblemListRequestId) {
        return
      }

      const responseProblems = Array.isArray(response.problems) ? response.problems : []
      const filteredProblems = activeProblemIdSearch == null
        ? responseProblems
        : responseProblems.filter((problem) => Number(problem.problem_id ?? 0) === activeProblemIdSearch)

      problems.value = filteredProblems.map((problem) => ({
        problem_id: Number(problem.problem_id ?? 0),
        title: problem.title ?? '',
        version: Number(problem.version ?? 0),
        time_limit_ms: Number(problem.time_limit_ms ?? 0),
        memory_limit_mb: Number(problem.memory_limit_mb ?? 0)
      }))

      if (!problems.value.length) {
        selectedProblemId.value = 0
        selectedProblemDetail.value = null
        detailErrorMessage.value = ''
        resetEditorDrafts()
        return
      }

      const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
        ? preferredProblemId
        : problems.value[0].problem_id

      void selectProblem(nextProblemId, {
        force: nextProblemId !== selectedProblemId.value || !selectedProblemDetail.value
      })
    } catch (error) {
      if (requestId !== latestProblemListRequestId) {
        return
      }

      problems.value = []
      selectedProblemId.value = 0
      selectedProblemDetail.value = null
      resetEditorDrafts()
      listErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemListRequestId) {
        isLoadingProblems.value = false
      }
    }
  }

  async function loadSelectedProblem(problemId){
    if (!problemId) {
      return
    }

    const requestId = ++latestProblemDetailRequestId
    selectedProblemId.value = problemId
    isLoadingDetail.value = true
    detailErrorMessage.value = ''
    actionErrorMessage.value = ''
    selectedProblemDetail.value = null
    resetEditorDrafts()

    try {
      const response = await getProblemDetail(problemId, {
        bearerToken: authState.token || ''
      })

      if (requestId !== latestProblemDetailRequestId) {
        return
      }

      const normalizedDetail = normalizeProblemDetail(response)
      selectedProblemDetail.value = normalizedDetail
      mergeProblemSummary(problemId, {
        title: normalizedDetail.title,
        version: normalizedDetail.version
      })
      assignEditorDrafts(normalizedDetail)
    } catch (error) {
      if (requestId !== latestProblemDetailRequestId) {
        return
      }

      detailErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 정보를 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemDetailRequestId) {
        isLoadingDetail.value = false
      }
    }
  }

  async function selectProblem(problemId, options = {}){
    if (!problemId) {
      return
    }

    if (!options.force && selectedProblemId.value === problemId && selectedProblemDetail.value) {
      return
    }

    await loadSelectedProblem(problemId)
  }

  function submitSearch(){
    if (searchMode.value === 'problem-id') {
      const nextProblemId = parsePositiveInteger(problemIdSearchInput.value)
      if (nextProblemId == null) {
        showErrorNotice('문제 번호를 입력하세요.')
        return
      }

      const nextQuery = buildSearchQuery('problem-id', {
        problemId: nextProblemId
      })
      void applySearchQuery(nextQuery, nextProblemId)
      return
    }

    const nextTitle = titleSearchInput.value.trim()
    const nextQuery = buildSearchQuery('title', {
      title: nextTitle
    })
    void applySearchQuery(nextQuery, selectedProblemId.value)
  }

  function resetSearch(){
    searchMode.value = 'title'
    titleSearchInput.value = ''
    problemIdSearchInput.value = ''
    void applySearchQuery(buildSearchQuery('title'), selectedProblemId.value)
  }

  async function applySearchQuery(nextQuery, preferredProblemId){
    if (isSameSearchQuery(nextQuery)) {
      await loadProblems({ preferredProblemId })
      return
    }

    await router.replace({
      query: nextQuery
    })
  }

  async function handleCreateProblem(){
    if (!authState.token || !canCreateProblem.value) {
      return
    }

    busySection.value = 'create'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await createProblem({
        title: newProblemTitle.value.trim()
      }, authState.token)

      newProblemTitle.value = ''
      searchMode.value = 'title'
      titleSearchInput.value = ''
      problemIdSearchInput.value = ''
      actionMessage.value = `문제 #${formatCount(response.problem_id)}를 생성했습니다.`

      await router.replace({
        query: {
          problemId: String(response.problem_id ?? '')
        }
      })
      await loadProblems({
        preferredProblemId: Number(response.problem_id ?? 0)
      })
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제를 생성하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveTitle(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveTitle.value) {
      return
    }

    busySection.value = 'title'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const nextTitle = titleDraft.value.trim()
      const response = await updateProblemTitle(selectedProblemDetail.value.problem_id, {
        title: nextTitle
      }, authState.token)

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        title: nextTitle
      }
      titleDraft.value = nextTitle
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      mergeProblemSummary(selectedProblemDetail.value.problem_id, {
        title: nextTitle
      })
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제목을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 제목을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveLimits(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveLimits.value) {
      return
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)
    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    busySection.value = 'limits'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await updateProblemLimits(selectedProblemDetail.value.problem_id, {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      }, authState.token)

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        limits: {
          time_limit_ms: nextTimeLimit,
          memory_limit_mb: nextMemoryLimit
        }
      }
      timeLimitDraft.value = String(nextTimeLimit)
      memoryLimitDraft.value = String(nextMemoryLimit)
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      mergeProblemSummary(selectedProblemDetail.value.problem_id, {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      })
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제한을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 제한을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveStatement(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveStatement.value) {
      return
    }

    busySection.value = 'statement'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const nextStatement = {
        description: descriptionDraft.value,
        input_format: inputFormatDraft.value,
        output_format: outputFormatDraft.value,
        note: noteDraft.value === '' ? null : noteDraft.value
      }

      const response = await updateProblemStatement(
        selectedProblemDetail.value.problem_id,
        nextStatement,
        authState.token
      )

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        statement: {
          description: descriptionDraft.value,
          input_format: inputFormatDraft.value,
          output_format: outputFormatDraft.value,
          note: noteDraft.value
        }
      }
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 설명을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 설명을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleCreateSample(){
    if (!authState.token || !selectedProblemDetail.value || !canCreateSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    busySection.value = 'sample:create'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await createProblemSample(problemId, authState.token)

      const nextSampleOrder = Number(response.sample_order ?? selectedProblemDetail.value.samples.length + 1)
      setSelectedProblemSamples([
        ...selectedProblemDetail.value.samples,
        {
          sample_order: nextSampleOrder,
          sample_input: '',
          sample_output: ''
        }
      ])
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 추가하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveSample(sampleOrder){
    if (!authState.token || !selectedProblemDetail.value || !canSaveSample(sampleOrder)) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!sampleDraft) {
      return
    }

    busySection.value = makeSampleBusyKey(sampleOrder)
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await updateProblemSample(problemId, sampleOrder, {
        sample_input: sampleDraft.sample_input,
        sample_output: sampleDraft.sample_output
      }, authState.token)

      setSelectedProblemSamples(selectedProblemDetail.value.samples.map((sample) =>
        sample.sample_order === sampleOrder
          ? {
            ...sample,
            sample_input: response.sample_input ?? sampleDraft.sample_input,
            sample_output: response.sample_output ?? sampleDraft.sample_output
          }
          : sample
      ))
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(sampleOrder)}를 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteLastSample(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteLastSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const lastSample = selectedProblemDetail.value.samples[selectedProblemDetail.value.samples.length - 1]
    if (!lastSample) {
      return
    }

    busySection.value = 'sample:delete-last'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await deleteProblemSample(problemId, authState.token)
      setSelectedProblemSamples(selectedProblemDetail.value.samples.filter(
        (sample) => sample.sample_order !== lastSample.sample_order
      ))
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 삭제하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  function handleTestcaseZipFileChange(event){
    const nextFile = event.target?.files?.[0] || null
    if (!nextFile) {
      testcaseZipFile.value = null
      return
    }

    if (!nextFile.name.toLowerCase().endsWith('.zip')) {
      testcaseZipFile.value = null
      testcaseZipInputKey.value += 1
      actionMessage.value = ''
      actionErrorMessage.value = 'ZIP 파일만 업로드할 수 있습니다.'
      return
    }

    testcaseZipFile.value = nextFile
    actionErrorMessage.value = ''
  }

  async function handleUploadTestcaseZip(){
    if (!authState.token || !selectedProblemDetail.value || !canUploadTestcaseZip.value || !testcaseZipFile.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const uploadFile = testcaseZipFile.value
    busySection.value = 'testcase:upload'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await uploadProblemTestcaseZip(problemId, uploadFile, authState.token)
      applySelectedProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId)

      const testcaseCount = Number(response.testcase_count ?? 0)
      actionMessage.value = `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  function openDeleteDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
    deleteDialogOpen.value = true
  }

  function openRejudgeDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
    rejudgeDialogOpen.value = true
  }

  function closeDeleteDialog(force = false){
    if (!force && isDeletingProblem.value) {
      return
    }

    deleteDialogOpen.value = false
    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
  }

  function closeRejudgeDialog(force = false){
    if (!force && isRejudgingProblem.value) {
      return
    }

    rejudgeDialogOpen.value = false
    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
  }

  async function handleRejudgeProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canRejudgeSelectedProblem.value) {
      return
    }

    const rejudgingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = 'rejudge'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      await rejudgeProblem(rejudgingProblemId, authState.token)
      closeRejudgeDialog(true)
      actionMessage.value = `문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 재채점을 요청하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteSelectedProblem.value) {
      return
    }

    const deletingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = 'delete'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      await deleteProblem(deletingProblemId, authState.token)
      closeDeleteDialog(true)
      actionMessage.value = `문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`
      await loadProblems()
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제를 삭제하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  function parsePositiveInteger(value){
    const numericValue = Number.parseInt(String(value), 10)
    return Number.isInteger(numericValue) && numericValue > 0 ? numericValue : null
  }

  async function refreshProblems(){
    await loadProblems({
      preferredProblemId: selectedProblemId.value
    })
  }

  onMounted(() => {
    void initializeAuth()
  })

  return {
    authState,
    isAuthenticated,
    canManageProblems,
    isLoadingProblems,
    isLoadingDetail,
    listErrorMessage,
    detailErrorMessage,
    searchMode,
    titleSearchInput,
    problemIdSearchInput,
    newProblemTitle,
    problems,
    selectedProblemId,
    selectedProblemDetail,
    titleDraft,
    timeLimitDraft,
    memoryLimitDraft,
    descriptionDraft,
    inputFormatDraft,
    outputFormatDraft,
    noteDraft,
    sampleDrafts,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    busySection,
    rejudgeDialogOpen,
    rejudgeConfirmProblemId,
    rejudgeConfirmTitle,
    deleteDialogOpen,
    deleteConfirmProblemId,
    deleteConfirmTitle,
    problemCount,
    canCreateProblem,
    isCreatingProblem,
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    isCreatingSample,
    isDeletingLastSample,
    isUploadingTestcaseZip,
    isRejudgingProblem,
    isDeletingProblem,
    canCreateSample,
    canUploadTestcaseZip,
    canDeleteLastSample,
    canSaveTitle,
    canSaveLimits,
    canSaveStatement,
    canDeleteSelectedProblem,
    canRejudgeSelectedProblem,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    toolbarStatusLabel,
    toolbarStatusTone,
    formatCount,
    formatProblemLimit,
    isSavingSample,
    canSaveSample,
    isLastSample,
    setSearchMode,
    handleProblemIdSearchInput,
    submitSearch,
    resetSearch,
    refreshProblems,
    selectProblem,
    handleCreateProblem,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample,
    handleTestcaseZipFileChange,
    handleUploadTestcaseZip,
    openDeleteDialog,
    openRejudgeDialog,
    closeDeleteDialog,
    closeRejudgeDialog,
    handleRejudgeProblem,
    handleDeleteProblem
  }
}
