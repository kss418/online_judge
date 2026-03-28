import { computed, nextTick, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import {
  createProblemTestcase,
  deleteProblemTestcase,
  getProblemDetail,
  getProblemList,
  getProblemTestcases,
  uploadProblemTestcaseZip,
  updateProblemTestcase
} from '@/api/problem'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'

export function useAdminProblemTestcasesPage(){
  const route = useRoute()
  const router = useRouter()
  const { authState, isAuthenticated, initializeAuth } = useAuth()
  const { showErrorNotice, showSuccessNotice } = useNotice()
  const countFormatter = new Intl.NumberFormat()

  const selectedProblemId = computed(() => {
    const parsedValue = Number.parseInt(route.params.problemId, 10)
    return Number.isInteger(parsedValue) && parsedValue > 0 ? parsedValue : 0
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

  const isLoadingProblems = ref(true)
  const isLoadingProblem = ref(false)
  const isLoadingTestcases = ref(false)
  const listErrorMessage = ref('')
  const problemErrorMessage = ref('')
  const testcaseErrorMessage = ref('')
  const searchMode = ref('title')
  const titleSearchInput = ref('')
  const problemIdSearchInput = ref('')
  const problems = ref([])
  const problemDetail = ref(null)
  const testcaseItems = ref([])
  const newTestcaseInput = ref('')
  const newTestcaseOutput = ref('')
  const testcaseZipFile = ref(null)
  const testcaseZipInputKey = ref(0)
  const selectedTestcaseOrder = ref(0)
  const selectedTestcaseInputDraft = ref('')
  const selectedTestcaseOutputDraft = ref('')
  const viewTestcaseOrderInput = ref('')
  const busySection = ref('')
  const testcaseSummaryElementMap = new Map()

  let latestProblemListRequestId = 0
  let latestProblemRequestId = 0
  let latestTestcaseRequestId = 0
  let latestProblemLimitHydrationId = 0

  const canManageProblems = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
  const problemCount = computed(() => problems.value.length)
  const testcaseCount = computed(() => testcaseItems.value.length)
  const toolbarStatusLabel = computed(() => {
    if (isLoadingProblems.value || isLoadingProblem.value || isLoadingTestcases.value) {
      return 'Loading'
    }

    if (selectedProblemId.value > 0) {
      return `${formatCount(testcaseCount.value)} Testcases`
    }

    return `${formatCount(problemCount.value)} Problems`
  })
  const toolbarStatusTone = computed(() => {
    if (listErrorMessage.value || problemErrorMessage.value || testcaseErrorMessage.value) {
      return 'danger'
    }

    return 'success'
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
  const isCreatingTestcase = computed(() => busySection.value === 'create')
  const isUploadingTestcaseZip = computed(() => busySection.value === 'upload')
  const isDeletingLastTestcase = computed(() => busySection.value === 'delete-last')
  const isSavingSelectedTestcase = computed(() => busySection.value === 'save')
  const selectedTestcaseZipName = computed(() => testcaseZipFile.value?.name || '')
  const selectedTestcase = computed(() =>
    testcaseItems.value.find((testcase) => testcase.testcase_order === selectedTestcaseOrder.value) || null
  )
  const canCreateTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canUploadTestcaseZip = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )
  const canDeleteLastTestcase = computed(() =>
    selectedProblemId.value > 0 &&
    Boolean(authState.token) &&
    Boolean(testcaseItems.value.length) &&
    !busySection.value
  )
  const canSaveSelectedTestcase = computed(() => {
    if (!selectedTestcase.value || !authState.token || busySection.value) {
      return false
    }

    return (
      selectedTestcaseInputDraft.value !== selectedTestcase.value.testcase_input ||
      selectedTestcaseOutputDraft.value !== selectedTestcase.value.testcase_output
    )
  })
  const canViewSpecificTestcase = computed(() => {
    if (isLoadingTestcases.value || !testcaseItems.value.length) {
      return false
    }

    return parsePositiveInteger(viewTestcaseOrderInput.value) != null
  })

  watch(selectedProblemId, () => {
    resetSelectedProblemState()

    if (!authState.initialized || !canManageProblems.value) {
      return
    }

    if (selectedProblemId.value > 0) {
      void loadSelectedProblemData()
      return
    }

    isLoadingProblem.value = false
    isLoadingTestcases.value = false
  })

  watch(selectedTestcase, (testcase) => {
    selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
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

  function formatCount(value){
    return countFormatter.format(Number(value) || 0)
  }

  function formatProblemLimit(value, unit){
    const numericValue = Number(value ?? 0)
    if (!numericValue) {
      return `미설정 ${unit}`
    }

    return `${formatCount(numericValue)} ${unit}`
  }

  function normalizeProblemItem(problem){
    return {
      ...problem,
      problem_id: Number(problem.problem_id ?? 0),
      version: Number(problem.version ?? 1),
      title: problem.title || '',
      time_limit_ms: Number(problem.time_limit_ms ?? problem.limits?.time_limit_ms ?? 0),
      memory_limit_mb: Number(problem.memory_limit_mb ?? problem.limits?.memory_limit_mb ?? 0)
    }
  }

  function normalizeProblemDetail(detail){
    return {
      ...detail,
      problem_id: Number(detail.problem_id ?? 0),
      title: detail.title || ''
    }
  }

  function normalizeTestcaseList(response){
    const rawTestcases = Array.isArray(response?.testcases) ? response.testcases : []

    return rawTestcases.map((testcase) => ({
      testcase_id: Number(testcase.testcase_id ?? 0),
      testcase_order: Number(testcase.testcase_order ?? 0),
      testcase_input: typeof testcase.testcase_input === 'string' ? testcase.testcase_input : '',
      testcase_output: typeof testcase.testcase_output === 'string' ? testcase.testcase_output : ''
    }))
  }

  function parsePositiveInteger(value){
    const normalized = typeof value === 'string' ? value.trim() : String(value ?? '').trim()
    if (!normalized) {
      return null
    }

    const parsedValue = Number.parseInt(normalized, 10)
    if (!Number.isInteger(parsedValue) || parsedValue <= 0) {
      return null
    }

    return parsedValue
  }

  function describeTestcaseContent(value){
    if (!value) {
      return '빈 값'
    }

    return `${formatCount(value.length)}자 · ${formatCount(value.split('\n').length)}줄`
  }

  function isLastTestcase(testcaseOrder){
    if (!testcaseItems.value.length) {
      return false
    }

    return testcaseItems.value[testcaseItems.value.length - 1].testcase_order === testcaseOrder
  }

  function resetSelectedProblemState(){
    problemDetail.value = null
    testcaseItems.value = []
    newTestcaseInput.value = ''
    newTestcaseOutput.value = ''
    testcaseZipFile.value = null
    testcaseZipInputKey.value += 1
    selectedTestcaseOrder.value = 0
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    viewTestcaseOrderInput.value = ''
    problemErrorMessage.value = ''
    testcaseErrorMessage.value = ''
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

  function syncSelectedTestcase(preferredOrder){
    if (!testcaseItems.value.length) {
      selectedTestcaseOrder.value = 0
      selectedTestcaseInputDraft.value = ''
      selectedTestcaseOutputDraft.value = ''
      viewTestcaseOrderInput.value = ''
      return
    }

    const lastTestcaseOrder = testcaseItems.value[testcaseItems.value.length - 1].testcase_order
    const candidateOrder =
      preferredOrder && preferredOrder > 0
        ? Math.min(preferredOrder, lastTestcaseOrder)
        : selectedTestcaseOrder.value > 0
          ? Math.min(selectedTestcaseOrder.value, lastTestcaseOrder)
          : testcaseItems.value[0].testcase_order
    const matchedTestcase = testcaseItems.value.find(
      (testcase) => testcase.testcase_order === candidateOrder
    )
    const nextOrder = matchedTestcase
      ? matchedTestcase.testcase_order
      : testcaseItems.value[0].testcase_order

    selectedTestcaseOrder.value = nextOrder
    viewTestcaseOrderInput.value = String(nextOrder)
    void scrollSelectedTestcaseIntoView()
  }

  function selectTestcase(testcaseOrder){
    selectedTestcaseOrder.value = testcaseOrder
    viewTestcaseOrderInput.value = String(testcaseOrder)
    void scrollSelectedTestcaseIntoView()
  }

  function setTestcaseSummaryElement(testcaseOrder, element){
    if (!element) {
      testcaseSummaryElementMap.delete(testcaseOrder)
      return
    }

    testcaseSummaryElementMap.set(testcaseOrder, element)
  }

  async function scrollSelectedTestcaseIntoView(){
    await nextTick()

    const summaryElement = testcaseSummaryElementMap.get(selectedTestcaseOrder.value)
    if (!summaryElement || typeof summaryElement.scrollIntoView !== 'function') {
      return
    }

    summaryElement.scrollIntoView({
      block: 'nearest',
      inline: 'nearest'
    })
  }

  async function selectProblem(problemId){
    if (problemId === selectedProblemId.value) {
      return
    }

    await router.push({
      name: 'admin-problem-testcases',
      params: {
        problemId: String(problemId)
      },
      query: buildSearchQuery(routeSearchMode.value, {
        title: routeTitleSearch.value,
        problemId: routeProblemIdSearch.value
      })
    })
  }

  function submitSearch(){
    if (searchMode.value === 'problem-id') {
      const nextProblemId = parsePositiveInteger(problemIdSearchInput.value)
      if (nextProblemId == null) {
        showErrorNotice('문제 번호를 입력하세요.')
        return
      }

      void applySearchQuery(buildSearchQuery('problem-id', {
        problemId: nextProblemId
      }), nextProblemId)
      return
    }

    void applySearchQuery(buildSearchQuery('title', {
      title: titleSearchInput.value
    }), selectedProblemId.value)
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
      name: 'admin-problem-testcases',
      params: selectedProblemId.value > 0
        ? {
          problemId: String(selectedProblemId.value)
        }
        : {},
      query: nextQuery
    })
  }

  function handleViewSelectedTestcase(){
    const targetOrder = parsePositiveInteger(viewTestcaseOrderInput.value)
    if (targetOrder == null) {
      showErrorNotice('확인할 테스트케이스 순번을 올바르게 입력하세요.')
      return
    }

    const matchedTestcase = testcaseItems.value.find(
      (testcase) => testcase.testcase_order === targetOrder
    )
    if (!matchedTestcase) {
      showErrorNotice(`테스트케이스 ${targetOrder}번이 없습니다.`)
      return
    }

    selectTestcase(matchedTestcase.testcase_order)
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
      showErrorNotice('ZIP 파일만 업로드할 수 있습니다.')
      return
    }

    testcaseZipFile.value = nextFile
  }

  async function loadProblems(options = {}){
    const requestId = ++latestProblemListRequestId
    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
    isLoadingProblems.value = true
    listErrorMessage.value = ''

    try {
      const activeTitleSearch = routeSearchMode.value === 'title'
        ? routeTitleSearch.value
        : ''
      const activeProblemIdSearch = routeSearchMode.value === 'problem-id'
        ? routeProblemIdSearch.value
        : null
      const response = await getProblemList({
        title: activeTitleSearch,
        bearerToken: authState.token || ''
      })

      if (requestId !== latestProblemListRequestId) {
        return
      }

      const responseProblems = Array.isArray(response.problems) ? response.problems : []
      const filteredProblems = activeProblemIdSearch == null
        ? responseProblems
        : responseProblems.filter((problem) => Number(problem.problem_id ?? 0) === activeProblemIdSearch)

      problems.value = filteredProblems.map(normalizeProblemItem)

      if (!problems.value.length) {
        if (selectedProblemId.value > 0) {
          await router.replace({
            name: 'admin-problem-testcases',
            query: buildSearchQuery(routeSearchMode.value, {
              title: routeTitleSearch.value,
              problemId: routeProblemIdSearch.value
            })
          })
        } else {
          problemDetail.value = null
          testcaseItems.value = []
          problemErrorMessage.value = ''
          testcaseErrorMessage.value = ''
          isLoadingProblem.value = false
          isLoadingTestcases.value = false
        }
        return
      }

      void hydrateProblemLimits(problems.value.map((problem) => problem.problem_id))

      const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
        ? preferredProblemId
        : problems.value[0].problem_id
      if (nextProblemId > 0 && nextProblemId !== selectedProblemId.value) {
        await router.replace({
          name: 'admin-problem-testcases',
          params: {
            problemId: String(nextProblemId)
          },
          query: buildSearchQuery(routeSearchMode.value, {
            title: routeTitleSearch.value,
            problemId: routeProblemIdSearch.value
          })
        })
      }
    } catch (error) {
      if (requestId !== latestProblemListRequestId) {
        return
      }

      problems.value = []
      listErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 목록을 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemListRequestId) {
        isLoadingProblems.value = false
      }
    }
  }

  async function hydrateProblemLimits(problemIds){
    if (!problemIds.length) {
      return
    }

    const hydrationId = ++latestProblemLimitHydrationId
    const detailEntries = await Promise.all(problemIds.map(async (problemId) => {
      try {
        const response = await getProblemDetail(problemId, {
          bearerToken: authState.token || ''
        })
        const detail = normalizeProblemDetail(response)
        return [
          problemId,
          {
            time_limit_ms: detail.limits?.time_limit_ms ?? 0,
            memory_limit_mb: detail.limits?.memory_limit_mb ?? 0
          }
        ]
      } catch {
        return null
      }
    }))

    if (hydrationId !== latestProblemLimitHydrationId) {
      return
    }

    const limitMap = new Map(detailEntries.filter(Boolean))
    if (!limitMap.size) {
      return
    }

    problems.value = problems.value.map((problem) => {
      const limitPatch = limitMap.get(problem.problem_id)
      return limitPatch
        ? {
          ...problem,
          ...limitPatch
        }
        : problem
    })
  }

  async function loadProblemDetail(){
    const requestId = ++latestProblemRequestId
    isLoadingProblem.value = true
    problemErrorMessage.value = ''

    if (selectedProblemId.value <= 0) {
      problemDetail.value = null
      isLoadingProblem.value = false
      return
    }

    try {
      const response = await getProblemDetail(selectedProblemId.value, {
        bearerToken: authState.token || ''
      })

      if (requestId !== latestProblemRequestId) {
        return
      }

      problemDetail.value = normalizeProblemDetail(response)
    } catch (error) {
      if (requestId !== latestProblemRequestId) {
        return
      }

      problemDetail.value = null
      problemErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 정보를 불러오지 못했습니다.'
    } finally {
      if (requestId === latestProblemRequestId) {
        isLoadingProblem.value = false
      }
    }
  }

  async function loadTestcases(){
    const requestId = ++latestTestcaseRequestId
    isLoadingTestcases.value = true
    testcaseErrorMessage.value = ''

    if (!authState.token || selectedProblemId.value <= 0) {
      testcaseItems.value = []
      isLoadingTestcases.value = false
      return
    }

    try {
      const response = await getProblemTestcases(selectedProblemId.value, {
        bearerToken: authState.token
      })

      if (requestId !== latestTestcaseRequestId) {
        return
      }

      testcaseItems.value = normalizeTestcaseList(response)
      syncSelectedTestcase()
    } catch (error) {
      if (requestId !== latestTestcaseRequestId) {
        return
      }

      testcaseItems.value = []
      selectedTestcaseOrder.value = 0
      selectedTestcaseInputDraft.value = ''
      selectedTestcaseOutputDraft.value = ''
      viewTestcaseOrderInput.value = ''
      testcaseErrorMessage.value = error instanceof Error
        ? error.message
        : '테스트케이스를 불러오지 못했습니다.'
    } finally {
      if (requestId === latestTestcaseRequestId) {
        isLoadingTestcases.value = false
      }
    }
  }

  async function loadSelectedProblemData(){
    if (selectedProblemId.value <= 0) {
      isLoadingProblem.value = false
      isLoadingTestcases.value = false
      return
    }

    await Promise.all([
      loadProblemDetail(),
      loadTestcases()
    ])
  }

  async function refreshPage(){
    if (busySection.value) {
      return
    }

    await loadProblems({
      preferredProblemId:
        routeSearchMode.value === 'problem-id'
          ? routeProblemIdSearch.value || selectedProblemId.value
          : selectedProblemId.value
    })
    await loadSelectedProblemData()
  }

  async function handleCreateTestcase(){
    if (!canCreateTestcase.value || !authState.token) {
      return
    }

    busySection.value = 'create'

    const nextTestcaseInput = newTestcaseInput.value
    const nextTestcaseOutput = newTestcaseOutput.value

    try {
      const response = await createProblemTestcase(
        selectedProblemId.value,
        {
          testcase_input: nextTestcaseInput,
          testcase_output: nextTestcaseOutput
        },
        authState.token
      )

      testcaseItems.value = [
        ...testcaseItems.value,
        {
          testcase_id: Number(response.testcase_id ?? 0),
          testcase_order: Number(response.testcase_order ?? (testcaseItems.value.length + 1)),
          testcase_input: nextTestcaseInput,
          testcase_output: nextTestcaseOutput
        }
      ]
      syncSelectedTestcase(Number(response.testcase_order ?? 0))
      newTestcaseInput.value = ''
      newTestcaseOutput.value = ''
      showSuccessNotice('테스트케이스를 마지막에 추가했습니다.')
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 추가하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleUploadTestcaseZip(){
    if (!canUploadTestcaseZip.value || !authState.token || !testcaseZipFile.value) {
      return
    }

    busySection.value = 'upload'
    const uploadFile = testcaseZipFile.value

    try {
      const response = await uploadProblemTestcaseZip(selectedProblemId.value, uploadFile, authState.token)
      testcaseZipFile.value = null
      testcaseZipInputKey.value += 1
      await Promise.all([
        loadProblems(),
        loadSelectedProblemData()
      ])

      const uploadedTestcaseCount = Number(response.testcase_count ?? 0)
      showSuccessNotice(`테스트케이스 ${formatCount(uploadedTestcaseCount)}개를 업로드했습니다.`)
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스 ZIP을 업로드하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteLastTestcase(){
    if (!canDeleteLastTestcase.value || !authState.token) {
      return
    }

    busySection.value = 'delete-last'

    try {
      await deleteProblemTestcase(selectedProblemId.value, authState.token)
      testcaseItems.value = testcaseItems.value.slice(0, -1)
      syncSelectedTestcase(selectedTestcaseOrder.value)
      showSuccessNotice('마지막 테스트케이스를 삭제했습니다.')
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 삭제하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveSelectedTestcase(){
    if (!selectedTestcase.value || !canSaveSelectedTestcase.value || !authState.token) {
      return
    }

    busySection.value = 'save'

    const testcaseOrder = selectedTestcase.value.testcase_order
    const nextTestcaseInput = selectedTestcaseInputDraft.value
    const nextTestcaseOutput = selectedTestcaseOutputDraft.value

    try {
      const response = await updateProblemTestcase(
        selectedProblemId.value,
        testcaseOrder,
        {
          testcase_input: nextTestcaseInput,
          testcase_output: nextTestcaseOutput
        },
        authState.token
      )

      testcaseItems.value = testcaseItems.value.map((testcase) => {
        if (testcase.testcase_order !== testcaseOrder) {
          return testcase
        }

        return {
          ...testcase,
          testcase_id: Number(response.testcase_id ?? testcase.testcase_id),
          testcase_order: Number(response.testcase_order ?? testcase.testcase_order),
          testcase_input: typeof response.testcase_input === 'string'
            ? response.testcase_input
            : nextTestcaseInput,
          testcase_output: typeof response.testcase_output === 'string'
            ? response.testcase_output
            : nextTestcaseOutput
        }
      })
      showSuccessNotice(`테스트케이스 ${testcaseOrder}번을 저장했습니다.`)
    } catch (error) {
      showErrorNotice(
        error instanceof Error
          ? error.message
          : '테스트케이스를 저장하지 못했습니다.'
      )
    } finally {
      busySection.value = ''
    }
  }

  onMounted(async () => {
    await initializeAuth()

    if (!canManageProblems.value) {
      isLoadingProblems.value = false
      isLoadingProblem.value = false
      isLoadingTestcases.value = false
      return
    }

    syncSearchControlsFromRoute()
    await loadProblems({
      preferredProblemId:
        routeSearchMode.value === 'problem-id'
          ? routeProblemIdSearch.value || selectedProblemId.value
          : selectedProblemId.value
    })
    await loadSelectedProblemData()
  })

  return {
    authState,
    isAuthenticated,
    canManageProblems,
    selectedProblemId,
    isLoadingProblems,
    isLoadingProblem,
    isLoadingTestcases,
    listErrorMessage,
    problemErrorMessage,
    testcaseErrorMessage,
    searchMode,
    titleSearchInput,
    problemIdSearchInput,
    problems,
    problemDetail,
    testcaseItems,
    newTestcaseInput,
    newTestcaseOutput,
    testcaseZipInputKey,
    selectedTestcaseOrder,
    selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft,
    viewTestcaseOrderInput,
    busySection,
    problemCount,
    toolbarStatusLabel,
    toolbarStatusTone,
    hasAppliedSearch,
    problemListCaption,
    emptyProblemListMessage,
    isCreatingTestcase,
    isUploadingTestcaseZip,
    isDeletingLastTestcase,
    isSavingSelectedTestcase,
    selectedTestcaseZipName,
    selectedTestcase,
    canCreateTestcase,
    canUploadTestcaseZip,
    canDeleteLastTestcase,
    canSaveSelectedTestcase,
    canViewSpecificTestcase,
    formatCount,
    formatProblemLimit,
    describeTestcaseContent,
    isLastTestcase,
    setSearchMode,
    handleProblemIdSearchInput,
    submitSearch,
    resetSearch,
    selectProblem,
    refreshPage,
    handleTestcaseZipFileChange,
    handleUploadTestcaseZip,
    handleCreateTestcase,
    selectTestcase,
    handleDeleteLastTestcase,
    handleSaveSelectedTestcase,
    handleViewSelectedTestcase,
    setTestcaseSummaryElement
  }
}
