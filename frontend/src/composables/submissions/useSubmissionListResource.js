import { computed, ref } from 'vue'

import { getSupportedLanguages } from '@/api/http'
import { getSubmissionList } from '@/api/submission'
import { useAsyncResource } from '@/composables/useAsyncResource'
import {
  listLimit,
  normalizeSubmissionMetric,
  normalizeSubmittedAt
} from '@/composables/submissions/submissionHelpers'

function createInitialSubmissionListState(){
  return {
    submissions: []
  }
}

export function useSubmissionListResource({
  authState,
  isAuthenticated,
  authenticatedBearerToken,
  isMineScope,
  numericProblemId,
  activeUserId,
  activeUserLoginId,
  appliedLanguageFilter,
  appliedStatusFilter,
  selectedLanguageFilter,
  pagination,
  resetRejudgingSubmissions
}){
  const isLoadingLanguages = ref(true)
  const supportedSubmissionLanguages = ref([])
  const submissionListResource = useAsyncResource({
    initialData: createInitialSubmissionListState,
    async load({
      targetPageNumber,
      targetBeforeSubmissionId,
      targetPreviousBeforeSubmissionIds
    }){
      const response = await getSubmissionList({
        limit: listLimit,
        beforeSubmissionId: targetBeforeSubmissionId ?? undefined,
        problemId: numericProblemId.value ?? undefined,
        userId: activeUserId.value ?? undefined,
        userLoginId: activeUserLoginId.value || undefined,
        language: appliedLanguageFilter.value || undefined,
        status: appliedStatusFilter.value || undefined,
        bearerToken: authenticatedBearerToken.value
      })

      const normalizedSubmissions = Array.isArray(response.submissions)
        ? response.submissions
          .map((submission) => {
            const normalizedSubmittedAt = normalizeSubmittedAt(submission.created_at)

            return {
              ...submission,
              submission_id: Number(submission.submission_id),
              user_id: Number(submission.user_id),
              problem_id: Number(submission.problem_id),
              user_login_id:
                (typeof submission.user_login_id === 'string' && submission.user_login_id) ||
                `사용자 ${submission.user_id}`,
              created_at: typeof submission.created_at === 'string' ? submission.created_at : '',
              created_at_timestamp: normalizedSubmittedAt.timestamp,
              created_at_label: normalizedSubmittedAt.label,
              elapsed_ms: typeof submission.elapsed_ms === 'number' ? submission.elapsed_ms : null,
              max_rss_kb: typeof submission.max_rss_kb === 'number' ? submission.max_rss_kb : null
            }
          })
        : []
      const normalizedHasMore = Boolean(response.has_more)
      const normalizedNextBeforeSubmissionId =
        Number.isInteger(response.next_before_submission_id) &&
        response.next_before_submission_id > 0
          ? response.next_before_submission_id
          : null

      return {
        submissions: normalizedSubmissions,
        hasMoreSubmissions: normalizedHasMore,
        nextBeforeSubmissionId:
          normalizedHasMore && normalizedNextBeforeSubmissionId
            ? normalizedNextBeforeSubmissionId
            : null,
        currentBeforeSubmissionId: targetBeforeSubmissionId,
        previousBeforeSubmissionIds: targetPreviousBeforeSubmissionIds,
        currentPage: targetPageNumber
      }
    },
    getErrorMessage(error){
      return error instanceof Error
        ? error.message
        : '제출 목록을 불러오지 못했습니다.'
    }
  })

  const isLoading = submissionListResource.isLoading
  const errorMessage = submissionListResource.errorMessage
  const hasLoadedOnce = submissionListResource.hasLoadedOnce
  const submissions = computed(() => submissionListResource.data.value.submissions)
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

  function patchSubmission(submissionId, patch){
    submissionListResource.mutate((submissionListState) => ({
      ...submissionListState,
      submissions: submissionListState.submissions.map((submission) =>
        submission.submission_id === submissionId
          ? {
            ...submission,
            ...patch
          }
          : submission
      )
    }))
  }

  function mergeSubmissionStatusBatch(statusSnapshots){
    const patchBySubmissionId = new Map()

    statusSnapshots.forEach((statusSnapshot) => {
      const submissionId = Number(statusSnapshot?.submission_id)
      if (!Number.isInteger(submissionId) || submissionId <= 0) {
        return
      }

      patchBySubmissionId.set(submissionId, {
        status: typeof statusSnapshot.status === 'string' && statusSnapshot.status
          ? statusSnapshot.status
          : undefined,
        score: statusSnapshot.score ?? null,
        elapsed_ms: normalizeSubmissionMetric(statusSnapshot.elapsed_ms),
        max_rss_kb: normalizeSubmissionMetric(statusSnapshot.max_rss_kb)
      })
    })

    if (patchBySubmissionId.size === 0) {
      return
    }

    submissionListResource.mutate((submissionListState) => ({
      ...submissionListState,
      submissions: submissionListState.submissions.map((submission) => {
        const patch = patchBySubmissionId.get(submission.submission_id)
        if (!patch) {
          return submission
        }

        return {
          ...submission,
          status: patch.status ?? submission.status,
          score: patch.score,
          elapsed_ms: patch.elapsed_ms,
          max_rss_kb: patch.max_rss_kb
        }
      })
    }))
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

  async function loadSubmissions(options = {}){
    const targetPageNumber = Number.isInteger(options.pageNumber) && options.pageNumber > 0
      ? options.pageNumber
      : pagination.currentPage.value
    const targetBeforeSubmissionId =
      Number.isInteger(options.beforeSubmissionId) && options.beforeSubmissionId > 0
        ? options.beforeSubmissionId
        : null
    const targetPreviousBeforeSubmissionIds = Array.isArray(options.previousBeforeSubmissionIds)
      ? [...options.previousBeforeSubmissionIds]
      : [...pagination.previousBeforeSubmissionIds.value]

    isLoading.value = true
    errorMessage.value = ''
    resetRejudgingSubmissions()

    if (isMineScope.value && !isAuthenticated.value) {
      submissionListResource.mutate(createInitialSubmissionListState())
      pagination.hasMoreSubmissions.value = false
      pagination.nextBeforeSubmissionId.value = null
      errorMessage.value = '내 제출을 보려면 로그인하세요.'
      hasLoadedOnce.value = true
      isLoading.value = false
      return
    }

    const result = await submissionListResource.run({
      targetPageNumber,
      targetBeforeSubmissionId,
      targetPreviousBeforeSubmissionIds
    }, {
      resetDataOnError: true
    })

    if (result.status === 'error') {
      pagination.hasMoreSubmissions.value = false
      pagination.nextBeforeSubmissionId.value = null
      return
    }

    if (result.status !== 'success') {
      return
    }

    pagination.hasMoreSubmissions.value = result.data.hasMoreSubmissions
    pagination.nextBeforeSubmissionId.value = result.data.nextBeforeSubmissionId
    pagination.currentBeforeSubmissionId.value = result.data.currentBeforeSubmissionId
    pagination.previousBeforeSubmissionIds.value = result.data.previousBeforeSubmissionIds
    pagination.currentPage.value = result.data.currentPage
  }

  async function refreshSubmissions(){
    await loadSubmissions({
      pageNumber: pagination.currentPage.value,
      beforeSubmissionId: pagination.currentBeforeSubmissionId.value,
      previousBeforeSubmissionIds: pagination.previousBeforeSubmissionIds.value
    })
  }

  return {
    isLoading,
    isLoadingLanguages,
    errorMessage,
    submissions,
    hasLoadedOnce,
    submissionLanguageFilterOptions,
    patchSubmission,
    mergeSubmissionStatusBatch,
    loadSupportedSubmissionLanguages,
    loadSubmissions,
    refreshSubmissions
  }
}
