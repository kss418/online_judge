import { computed, ref } from 'vue'

import { getProblemDetail } from '@/api/problemQueryApi'
import { getSupportedLanguages } from '@/api/system'
import { getMySubmissionBan } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { usePollingController } from '@/composables/usePollingController'
import { formatApiError } from '@/utils/apiError'
import {
  createSubmissionBanState,
  formatSubmissionBanRemaining,
  formatSubmissionBanUntil,
  isSubmissionBanActive as checkSubmissionBanActive
} from '@/utils/submissionBan'

export function useProblemSubmitResource({
  authState,
  isAuthenticated,
  numericProblemId
}){
  const nowTimestamp = ref(Date.now())
  const problemResource = useAsyncResource({
    initialData: null,
    async load(problemId){
      return getProblemDetail(problemId)
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '문제 정보를 불러오지 못했습니다.'
      })
    }
  })
  const languageResource = useAsyncResource({
    initialData: [],
    async load(){
      const response = await getSupportedLanguages()
      return Array.isArray(response.languages) ? response.languages : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '지원 언어를 불러오지 못했습니다.'
      })
    }
  })
  const submissionBanResource = useAsyncResource({
    initialData: createSubmissionBanState,
    async load(token){
      return getMySubmissionBan(token)
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '제출 제한 상태를 불러오지 못했습니다.'
      })
    }
  })

  usePollingController({
    task(){
      nowTimestamp.value = Date.now()
    },
    enabled: true,
    intervalMs: 30_000,
    pauseWhenHidden: false,
    runImmediately: true
  })

  const isLoadingProblem = problemResource.isLoading
  const problemErrorMessage = problemResource.errorMessage
  const problemDetail = problemResource.data
  const isLoadingLanguages = languageResource.isLoading
  const languageErrorMessage = languageResource.errorMessage
  const supportedLanguages = languageResource.data
  const submissionBan = submissionBanResource.data
  const isSubmissionBanActive = computed(() => (
    checkSubmissionBanActive(
      nowTimestamp.value,
      submissionBan.value.submission_banned_until_timestamp
    )
  ))
  const submissionBanUntilText = computed(() =>
    formatSubmissionBanUntil(submissionBan.value.submission_banned_until)
  )
  const submissionBanRemainingText = computed(() => {
    const remainingText = formatSubmissionBanRemaining(
      nowTimestamp.value,
      submissionBan.value.submission_banned_until_timestamp
    )

    if (!remainingText) {
      return ''
    }

    return `약 ${remainingText} 남음`
  })

  function resetProblemDetail(){
    problemResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadProblemDetail(){
    const normalizedProblemId = Number(numericProblemId.value)

    if (!Number.isInteger(normalizedProblemId) || normalizedProblemId <= 0) {
      resetProblemDetail()
      problemErrorMessage.value = '올바르지 않은 문제 번호입니다.'

      return {
        status: 'error'
      }
    }

    return problemResource.run(normalizedProblemId, {
      resetDataOnRun: true,
      resetDataOnError: true
    })
  }

  function loadSupportedLanguageList(){
    return languageResource.run(undefined, {
      resetDataOnError: true
    })
  }

  function resetSubmissionBan(){
    submissionBanResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadMySubmissionBan(){
    if (!authState.initialized || !isAuthenticated.value || !authState.token) {
      resetSubmissionBan()

      return {
        status: 'reset',
        data: submissionBan.value
      }
    }

    return submissionBanResource.run(authState.token, {
      resetDataOnError: true
    })
  }

  return {
    isLoadingProblem,
    problemErrorMessage,
    problemDetail,
    isLoadingLanguages,
    languageErrorMessage,
    supportedLanguages,
    isSubmissionBanActive,
    submissionBanUntilText,
    submissionBanRemainingText,
    loadProblemDetail,
    loadSupportedLanguageList,
    resetSubmissionBan,
    loadMySubmissionBan
  }
}
