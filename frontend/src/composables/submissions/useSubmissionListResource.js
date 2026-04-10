import { computed, ref } from 'vue'

import { getSupportedLanguages } from '@/api/http'
import { getSubmissionList } from '@/api/submission'
import {
  listLimit,
  normalizeSubmissionMetric,
  normalizeSubmittedAt
} from '@/composables/submissions/submissionHelpers'

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
  const isLoading = ref(true)
  const isLoadingLanguages = ref(true)
  const errorMessage = ref('')
  const submissions = ref([])
  const hasLoadedOnce = ref(false)
  const supportedSubmissionLanguages = ref([])

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

  let latestLoadRequestId = 0

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

    submissions.value = submissions.value.map((submission) => {
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
    const requestId = ++latestLoadRequestId
    isLoading.value = true
    errorMessage.value = ''
    resetRejudgingSubmissions()

    if (isMineScope.value && !isAuthenticated.value) {
      submissions.value = []
      pagination.hasMoreSubmissions.value = false
      pagination.nextBeforeSubmissionId.value = null
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
        beforeSubmissionId: targetBeforeSubmissionId ?? undefined,
        problemId: numericProblemId.value ?? undefined,
        userId: activeUserId.value ?? undefined,
        userLoginId: activeUserLoginId.value || undefined,
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

      submissions.value = normalizedSubmissions
      pagination.hasMoreSubmissions.value = normalizedHasMore
      pagination.nextBeforeSubmissionId.value =
        normalizedHasMore && normalizedNextBeforeSubmissionId
          ? normalizedNextBeforeSubmissionId
          : null
      pagination.currentBeforeSubmissionId.value = targetBeforeSubmissionId
      pagination.previousBeforeSubmissionIds.value = targetPreviousBeforeSubmissionIds
      pagination.currentPage.value = targetPageNumber
      hasLoadedOnce.value = true
    } catch (error) {
      if (requestId !== latestLoadRequestId) {
        return
      }

      errorMessage.value = error instanceof Error
        ? error.message
        : '제출 목록을 불러오지 못했습니다.'
      submissions.value = []
      pagination.hasMoreSubmissions.value = false
      pagination.nextBeforeSubmissionId.value = null
      hasLoadedOnce.value = true
    } finally {
      if (requestId === latestLoadRequestId) {
        isLoading.value = false
      }
    }
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
