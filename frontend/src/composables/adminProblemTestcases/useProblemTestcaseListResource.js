import { computed, ref } from 'vue'

import {
  getProblemDetail,
  getProblemList,
  getProblemTestcase,
  getProblemTestcases
} from '@/api/problem'
import {
  normalizeProblemDetail,
  normalizeProblemItem,
  normalizeTestcaseDetail,
  normalizeTestcaseList
} from '@/composables/adminProblemTestcases/testcaseHelpers'

export function useProblemTestcaseListResource({
  authState,
  selectedProblemId,
  routeSearchMode,
  routeTitleSearch,
  routeProblemIdSearch,
  replaceProblemRoute,
  syncSelectedTestcase,
  resetSelectedTestcaseState
}){
  const isLoadingProblems = ref(true)
  const isLoadingProblem = ref(false)
  const isLoadingTestcases = ref(false)
  const isLoadingSelectedTestcase = ref(false)
  const listErrorMessage = ref('')
  const problemErrorMessage = ref('')
  const testcaseErrorMessage = ref('')
  const selectedTestcaseErrorMessage = ref('')
  const problems = ref([])
  const problemDetail = ref(null)
  const testcaseItems = ref([])
  const selectedTestcase = ref(null)

  const problemCount = computed(() => problems.value.length)
  const testcaseCount = computed(() => testcaseItems.value.length)

  let latestProblemListRequestId = 0
  let latestProblemRequestId = 0
  let latestTestcaseRequestId = 0
  let latestSelectedTestcaseRequestId = 0

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

  function applyProblemVersion(problemId, version){
    const normalizedVersion = Number(version)
    if (!Number.isInteger(normalizedVersion) || normalizedVersion <= 0) {
      return
    }

    if (problemDetail.value?.problem_id === problemId) {
      problemDetail.value = {
        ...problemDetail.value,
        version: normalizedVersion
      }
    }

    mergeProblemSummary(problemId, {
      version: normalizedVersion
    })
  }

  function resetSelectedProblemResource(){
    latestSelectedTestcaseRequestId += 1
    problemDetail.value = null
    testcaseItems.value = []
    selectedTestcase.value = null
    problemErrorMessage.value = ''
    testcaseErrorMessage.value = ''
    selectedTestcaseErrorMessage.value = ''
    isLoadingSelectedTestcase.value = false
  }

  function clearSelectedTestcaseDetail(){
    latestSelectedTestcaseRequestId += 1
    selectedTestcase.value = null
    isLoadingSelectedTestcase.value = false
    selectedTestcaseErrorMessage.value = ''
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
          await replaceProblemRoute(0)
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

      const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
        ? preferredProblemId
        : problems.value[0].problem_id
      if (nextProblemId > 0 && nextProblemId !== selectedProblemId.value) {
        await replaceProblemRoute(nextProblemId)
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
      mergeProblemSummary(selectedProblemId.value, {
        title: problemDetail.value.title,
        version: problemDetail.value.version
      })
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

  async function loadSelectedTestcaseDetail(testcaseOrder){
    const requestId = ++latestSelectedTestcaseRequestId
    isLoadingSelectedTestcase.value = true
    selectedTestcaseErrorMessage.value = ''
    selectedTestcase.value = null

    try {
      const response = await getProblemTestcase(
        selectedProblemId.value,
        testcaseOrder,
        {
          bearerToken: authState.token || ''
        }
      )

      if (requestId !== latestSelectedTestcaseRequestId) {
        return
      }

      selectedTestcase.value = normalizeTestcaseDetail(response)
    } catch (error) {
      if (requestId !== latestSelectedTestcaseRequestId) {
        return
      }

      selectedTestcase.value = null
      selectedTestcaseErrorMessage.value = error instanceof Error
        ? error.message
        : '테스트케이스 본문을 불러오지 못했습니다.'
    } finally {
      if (requestId === latestSelectedTestcaseRequestId) {
        isLoadingSelectedTestcase.value = false
      }
    }
  }

  async function loadTestcases(preferredOrder){
    const requestId = ++latestTestcaseRequestId
    isLoadingTestcases.value = true
    testcaseErrorMessage.value = ''

    if (!authState.token || selectedProblemId.value <= 0) {
      testcaseItems.value = []
      selectedTestcase.value = null
      selectedTestcaseErrorMessage.value = ''
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
      syncSelectedTestcase(preferredOrder)
    } catch (error) {
      if (requestId !== latestTestcaseRequestId) {
        return
      }

      testcaseItems.value = []
      selectedTestcase.value = null
      selectedTestcaseErrorMessage.value = ''
      testcaseErrorMessage.value = error instanceof Error
        ? error.message
        : '테스트케이스를 불러오지 못했습니다.'
      resetSelectedTestcaseState()
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

  return {
    isLoadingProblems,
    isLoadingProblem,
    isLoadingTestcases,
    isLoadingSelectedTestcase,
    listErrorMessage,
    problemErrorMessage,
    testcaseErrorMessage,
    selectedTestcaseErrorMessage,
    problems,
    problemDetail,
    testcaseItems,
    selectedTestcase,
    problemCount,
    testcaseCount,
    mergeProblemSummary,
    applyProblemVersion,
    resetSelectedProblemResource,
    clearSelectedTestcaseDetail,
    loadProblems,
    loadProblemDetail,
    loadSelectedTestcaseDetail,
    loadTestcases,
    loadSelectedProblemData
  }
}
