import { computed, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'

import { getProblemDetail } from '@/api/problemQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { authStore } from '@/stores/auth/authStore'
import { formatApiError } from '@/utils/apiError'
import {
  getProblemStateLabel,
  getProblemStateTone
} from '@/utils/problemState'

const invalidProblemIdErrorCode = 'INVALID_PROBLEM_ID'

export function useProblemDetailPage(){
  const route = useRoute()
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const countFormatter = new Intl.NumberFormat()
  const rateFormatter = new Intl.NumberFormat('ko-KR', {
    minimumFractionDigits: 1,
    maximumFractionDigits: 1
  })
  const numericProblemId = computed(() => Number.parseInt(route.params.problemId, 10))
  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const problemDetailResource = useAsyncResource({
    initialData: () => null,
    async load({ problemId, bearerToken }){
      if (!Number.isInteger(problemId) || problemId <= 0) {
        const error = new Error('올바르지 않은 문제 번호입니다.')
        error.code = invalidProblemIdErrorCode
        throw error
      }

      return getProblemDetail(problemId, {
        bearerToken
      })
    },
    getErrorMessage(error){
      if (error?.code === invalidProblemIdErrorCode) {
        return error.message
      }

      return formatApiError(error, {
        fallback: '문제 정보를 불러오지 못했습니다.'
      })
    }
  })
  const problemDetail = problemDetailResource.data
  const isLoading = computed(() =>
    !problemDetailResource.hasLoadedOnce.value || problemDetailResource.isLoading.value
  )
  const acceptanceRate = computed(() => {
    const statistics = problemDetail.value?.statistics

    if (!statistics || statistics.submission_count <= 0) {
      return '-'
    }

    const rate = (statistics.accepted_count / statistics.submission_count) * 100
    return `${rateFormatter.format(rate)}%`
  })

  function loadProblemDetail(){
    return problemDetailResource.run({
      problemId: numericProblemId.value,
      bearerToken: authenticatedBearerToken.value
    }, {
      resetDataOnError: true
    })
  }

  function formatCount(value){
    return countFormatter.format(value)
  }

  watch(numericProblemId, () => {
    void loadProblemDetail()
  })

  onMounted(async () => {
    if (!authState.initialized) {
      await initializeAuth()
    }

    if (
      !problemDetailResource.hasLoadedOnce.value &&
      !problemDetailResource.isLoading.value
    ) {
      void loadProblemDetail()
    }
  })

  watch(authenticatedBearerToken, (nextToken, previousToken) => {
    if (nextToken === previousToken) {
      return
    }

    void loadProblemDetail()
  })

  return {
    isAuthenticated,
    isLoading,
    errorMessage: problemDetailResource.errorMessage,
    problemDetail,
    acceptanceRate,
    formatCount,
    getProblemStateLabel,
    getProblemStateTone
  }
}
