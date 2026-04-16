import { computed } from 'vue'

import {
  getProblemDetail,
  getProblemList
} from '@/api/problemQueryApi'
import {
  getProblemTestcase,
  getProblemTestcases
} from '@/api/testcaseApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { buildApiQuery as buildProblemAdminSearchApiQuery } from '@/queryState/problemAdminSearch'
import { formatApiError } from '@/utils/apiError'

export function useProblemTestcaseListResource({
  authState,
  selectedProblemId,
  routeQueryState
}){
  const problemListResource = useAsyncResource({
    initialData: [],
    async load({ routeQuery }){
      const apiQuery = buildProblemAdminSearchApiQuery(routeQuery)
      const response = await getProblemList({
        title: apiQuery.title,
        problemId: apiQuery.problemId,
        bearerToken: authState.token || ''
      })

      return response.problems
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 목록을 불러오지 못했습니다.'
      })
    }
  })
  const problemDetailResource = useAsyncResource({
    initialData: null,
    async load(problemId){
      return getProblemDetail(problemId, {
        bearerToken: authState.token || ''
      })
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 정보를 불러오지 못했습니다.'
      })
    }
  })
  const testcaseListResource = useAsyncResource({
    initialData: [],
    async load(problemId){
      const response = await getProblemTestcases(problemId, {
        bearerToken: authState.token
      })

      return response.testcases
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '테스트케이스를 불러오지 못했습니다.'
      })
    }
  })
  const selectedTestcaseResource = useAsyncResource({
    initialData: null,
    async load({ problemId, testcaseOrder }){
      return getProblemTestcase(problemId, testcaseOrder, {
        bearerToken: authState.token || ''
      })
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '테스트케이스 본문을 불러오지 못했습니다.'
      })
    }
  })

  const isLoadingProblems = problemListResource.isLoading
  const isLoadingProblem = problemDetailResource.isLoading
  const isLoadingTestcases = testcaseListResource.isLoading
  const isLoadingSelectedTestcase = selectedTestcaseResource.isLoading
  const listErrorMessage = problemListResource.errorMessage
  const problemErrorMessage = problemDetailResource.errorMessage
  const testcaseErrorMessage = testcaseListResource.errorMessage
  const selectedTestcaseErrorMessage = selectedTestcaseResource.errorMessage
  const problems = problemListResource.data
  const problemDetail = problemDetailResource.data
  const testcaseItems = testcaseListResource.data
  const selectedTestcase = selectedTestcaseResource.data

  const problemCount = computed(() => problems.value.length)
  const testcaseCount = computed(() => testcaseItems.value.length)

  function mergeProblemSummary(problemId, patch){
    problemListResource.mutate((problemItems) =>
      problemItems.map((problem) =>
        problem.problem_id === problemId
          ? {
            ...problem,
            ...patch
          }
          : problem
      )
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

  function setTestcaseItems(nextValueOrUpdater){
    testcaseListResource.mutate(nextValueOrUpdater)
  }

  function setSelectedTestcase(nextValueOrUpdater){
    selectedTestcaseResource.mutate(nextValueOrUpdater)
  }

  function resetSelectedProblemResource(){
    problemDetailResource.reset({
      preserveHasLoadedOnce: true
    })
    testcaseListResource.reset({
      preserveHasLoadedOnce: true
    })
    selectedTestcaseResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  function clearSelectedTestcaseDetail(){
    selectedTestcaseResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  async function loadProblems(){
    return problemListResource.run({
      routeQuery: routeQueryState.value
    }, {
      resetDataOnError: true
    })
  }

  function resetProblemListResource(){
    problemListResource.reset({
      preserveHasLoadedOnce: true
    })
  }

  async function loadProblemDetail(){
    if (selectedProblemId.value <= 0) {
      problemDetailResource.reset({
        preserveHasLoadedOnce: true
      })
      return {
        status: 'reset'
      }
    }

    const result = await problemDetailResource.run(selectedProblemId.value, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return result
    }

    mergeProblemSummary(selectedProblemId.value, {
      title: result.data.title,
      version: result.data.version
    })
    return result
  }

  async function loadSelectedTestcaseDetail(testcaseOrder){
    if (selectedProblemId.value <= 0 || !Number.isInteger(testcaseOrder) || testcaseOrder <= 0) {
      clearSelectedTestcaseDetail()
      return {
        status: 'reset'
      }
    }

    const result = await selectedTestcaseResource.run({
      problemId: selectedProblemId.value,
      testcaseOrder
    }, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    return result
  }

  async function loadTestcases(){
    if (!authState.token || selectedProblemId.value <= 0) {
      testcaseListResource.reset({
        preserveHasLoadedOnce: true
      })
      selectedTestcaseResource.reset({
        preserveHasLoadedOnce: true
      })
      return {
        status: 'reset'
      }
    }

    const result = await testcaseListResource.run(selectedProblemId.value, {
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      selectedTestcaseResource.reset({
        preserveHasLoadedOnce: true
      })
      return result
    }

    return result
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
    setTestcaseItems,
    setSelectedTestcase,
    resetProblemListResource,
    resetSelectedProblemResource,
    clearSelectedTestcaseDetail,
    loadProblems,
    loadProblemDetail,
    loadSelectedTestcaseDetail,
    loadTestcases
  }
}
