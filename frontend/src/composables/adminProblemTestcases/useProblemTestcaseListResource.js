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
  routeQueryState,
  replaceProblemRoute,
  syncSelectedTestcase,
  resetSelectedTestcaseState
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

  async function loadProblems(options = {}){
    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)

    const result = await problemListResource.run({
      routeQuery: routeQueryState.value
    }, {
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return
    }

    if (!problems.value.length) {
      if (selectedProblemId.value > 0) {
        await replaceProblemRoute(0)
      } else {
        resetSelectedProblemResource()
      }
      return
    }

    const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problems.value[0].problem_id
    if (nextProblemId > 0 && nextProblemId !== selectedProblemId.value) {
      await replaceProblemRoute(nextProblemId)
    }
  }

  async function loadProblemDetail(){
    if (selectedProblemId.value <= 0) {
      problemDetailResource.reset({
        preserveHasLoadedOnce: true
      })
      return
    }

    const result = await problemDetailResource.run(selectedProblemId.value, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return
    }

    mergeProblemSummary(selectedProblemId.value, {
      title: result.data.title,
      version: result.data.version
    })
  }

  async function loadSelectedTestcaseDetail(testcaseOrder){
    const result = await selectedTestcaseResource.run({
      problemId: selectedProblemId.value,
      testcaseOrder
    }, {
      resetDataOnRun: true,
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      return
    }
  }

  async function loadTestcases(preferredOrder){
    if (!authState.token || selectedProblemId.value <= 0) {
      testcaseListResource.reset({
        preserveHasLoadedOnce: true
      })
      selectedTestcaseResource.reset({
        preserveHasLoadedOnce: true
      })
      return
    }

    const result = await testcaseListResource.run(selectedProblemId.value, {
      resetDataOnError: true
    })

    if (result.status !== 'success') {
      selectedTestcaseResource.reset({
        preserveHasLoadedOnce: true
      })
      resetSelectedTestcaseState()
      return
    }

    syncSelectedTestcase(preferredOrder)
  }

  async function loadSelectedProblemData(){
    if (selectedProblemId.value <= 0) {
      problemDetailResource.reset({
        preserveHasLoadedOnce: true
      })
      testcaseListResource.reset({
        preserveHasLoadedOnce: true
      })
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
