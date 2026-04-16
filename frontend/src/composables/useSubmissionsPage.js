import { computed, onMounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useSubmissionActions } from '@/composables/submissions/useSubmissionActions'
import { useSubmissionDialogs } from '@/composables/submissions/useSubmissionDialogs'
import { useSubmissionFilterQuery } from '@/composables/submissions/useSubmissionFilterQuery'
import {
  formatElapsedMs,
  formatMemory,
  listLimit
} from '@/composables/submissions/submissionHelpers'
import { useSubmissionListResource } from '@/composables/submissions/useSubmissionListResource'
import { useSubmissionPagination } from '@/composables/submissions/useSubmissionPagination'
import { useSubmissionPolling } from '@/composables/submissions/useSubmissionPolling'
import { submissionStatusOptions } from '@/queryState/submissionFilters'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'

export function useSubmissionsPage(){
  const route = useRoute()
  const router = useRouter()
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const {
    showErrorNotice,
    showSuccessNotice
  } = noticeStore
  const authenticatedBearerToken = computed(() =>
    authState.initialized && isAuthenticated.value ? authState.token : ''
  )
  const currentUserId = computed(() => {
    const normalizedUserId = Number(authState.currentUser?.id)
    return Number.isInteger(normalizedUserId) && normalizedUserId > 0
      ? normalizedUserId
      : null
  })

  const query = useSubmissionFilterQuery({
    route,
    router,
    formatCount
  })
  const listResource = useSubmissionListResource({
    isAuthenticated,
    authenticatedBearerToken,
    isMineScope: query.isMineScope,
    routeState: query.routeState,
    numericProblemId: query.numericProblemId,
    currentUserId,
    appliedLanguageFilter: query.appliedLanguageFilter,
    selectedLanguageFilter: query.selectedLanguageFilter
  })
  const actions = useSubmissionActions({
    authState,
    formatCount,
    patchSubmission: listResource.patchSubmission
  })
  const pagination = useSubmissionPagination({
    submissions: listResource.submissions,
    formatCount
  })

  const dialogs = useSubmissionDialogs({
    authState,
    canManageSubmissionRejudge: actions.canManageSubmissionRejudge,
    patchSubmission: listResource.patchSubmission
  })

  const polling = useSubmissionPolling({
    authState,
    authenticatedBearerToken,
    canManageSubmissionRejudge: actions.canManageSubmissionRejudge,
    hasAppliedStatusFilter: query.hasAppliedStatusFilter,
    isLoading: listResource.isLoading,
    errorMessage: listResource.errorMessage,
    submissions: listResource.submissions,
    mergeSubmissionStatusBatch: listResource.mergeSubmissionStatusBatch,
    historyDialogOpen: dialogs.historyDialogOpen,
    isLoadingHistory: dialogs.isLoadingHistory,
    historyErrorMessage: dialogs.historyErrorMessage,
    activeHistorySubmissionId: dialogs.activeHistorySubmissionId,
    latestHistoryStatus: dialogs.latestHistoryStatus,
    fetchSubmissionHistory: dialogs.fetchSubmissionHistory
  })

  watch(actions.actionMessage, (message) => {
    if (message) {
      showSuccessNotice(message)
    }
  })

  watch(actions.actionErrorMessage, (message) => {
    if (message) {
      showErrorNotice(message, { duration: 5000 })
    }
  })

  watch(authenticatedBearerToken, (nextToken, previousToken) => {
    if (!authState.initialized || nextToken === previousToken) {
      return
    }

    if (query.isMineScope.value) {
      pagination.resetPagination()
    }

    void loadSubmissions()
  })

  watch(
    [
      () => route.name,
      query.numericProblemId,
      query.appliedUserLoginIdFilter,
      query.isMineScope,
      query.appliedLanguageFilter,
      query.appliedStatusFilter
    ],
    () => {
      if (!authState.initialized) {
        return
      }

      query.syncSelectedFiltersFromRoute()
      pagination.resetPagination()
      void loadSubmissions()
    }
  )

  function normalizeLoadOptions(options = {}){
    const currentLoadOptions = pagination.buildCurrentLoadOptions()

    return {
      pageNumber: Number.isInteger(options.pageNumber) && options.pageNumber > 0
        ? options.pageNumber
        : currentLoadOptions.pageNumber,
      beforeSubmissionId:
        Number.isInteger(options.beforeSubmissionId) && options.beforeSubmissionId > 0
          ? options.beforeSubmissionId
          : null,
      previousBeforeSubmissionIds: Array.isArray(options.previousBeforeSubmissionIds)
        ? [...options.previousBeforeSubmissionIds]
        : currentLoadOptions.previousBeforeSubmissionIds
    }
  }

  async function loadSubmissions(options = {}){
    const nextLoadOptions = normalizeLoadOptions(options)

    if (
      listResource.isLoading.value &&
      (
        nextLoadOptions.beforeSubmissionId === pagination.currentBeforeSubmissionId.value ||
        (!nextLoadOptions.beforeSubmissionId && !pagination.currentBeforeSubmissionId.value)
      )
    ) {
      return
    }

    actions.resetRejudgingSubmissions()

    const result = await listResource.loadSubmissions(nextLoadOptions)

    if (result?.status === 'success') {
      pagination.applyLoadedPage(result.data)
      return result
    }

    pagination.clearNextPageAvailability()
    return result
  }

  function goToPreviousPage(){
    if (listResource.isLoading.value) {
      return
    }

    const previousPageLoadOptions = pagination.buildPreviousPageLoadOptions()
    if (!previousPageLoadOptions) {
      return
    }

    void loadSubmissions(previousPageLoadOptions)
  }

  function goToNextPage(){
    if (listResource.isLoading.value) {
      return
    }

    const nextPageLoadOptions = pagination.buildNextPageLoadOptions()
    if (!nextPageLoadOptions) {
      return
    }

    void loadSubmissions(nextPageLoadOptions)
  }

  function refreshSubmissions(){
    return loadSubmissions(pagination.buildCurrentLoadOptions())
  }

  onMounted(async () => {
    query.syncSelectedFiltersFromRoute()
    void listResource.loadSupportedSubmissionLanguages()

    if (!authState.initialized) {
      await initializeAuth()
    }

    if (!listResource.hasLoadedOnce.value) {
      void loadSubmissions()
    }
  })

  return {
    authState,
    isAuthenticated,
    listLimit,
    submissionStatusOptions,
    isLoading: listResource.isLoading,
    isLoadingLanguages: listResource.isLoadingLanguages,
    errorMessage: listResource.errorMessage,
    submissions: listResource.submissions,
    currentPage: pagination.currentPage,
    submissionCount: pagination.submissionCount,
    hasPreviousPage: pagination.hasPreviousPage,
    hasMoreSubmissions: pagination.hasMoreSubmissions,
    historyDialogOpen: dialogs.historyDialogOpen,
    isLoadingHistory: dialogs.isLoadingHistory,
    historyErrorMessage: dialogs.historyErrorMessage,
    submissionHistoryEntries: dialogs.submissionHistoryEntries,
    activeHistorySubmissionId: dialogs.activeHistorySubmissionId,
    sourceDialogOpen: dialogs.sourceDialogOpen,
    isLoadingSource: dialogs.isLoadingSource,
    sourceErrorMessage: dialogs.sourceErrorMessage,
    sourceDetail: dialogs.sourceDetail,
    activeSourceSubmissionId: dialogs.activeSourceSubmissionId,
    copyButtonLabel: dialogs.copyButtonLabel,
    selectedProblemIdFilter: query.selectedProblemIdFilter,
    selectedUserLoginIdFilter: query.selectedUserLoginIdFilter,
    selectedStatusFilter: query.selectedStatusFilter,
    selectedLanguageFilter: query.selectedLanguageFilter,
    submissionLanguageFilterOptions: listResource.submissionLanguageFilterOptions,
    canManageSubmissionRejudge: actions.canManageSubmissionRejudge,
    hasFixedProblemId: query.hasFixedProblemId,
    showUserLoginIdFilter: query.showUserLoginIdFilter,
    canApplyFilters: query.canApplyFilters,
    canResetFilters: query.canResetFilters,
    visibleRangeText: pagination.visibleRangeText,
    numericProblemId: query.numericProblemId,
    shouldPollSubmissionHistory: polling.shouldPollSubmissionHistory,
    pageTitle: query.pageTitle,
    formatCount,
    formatRelativeSubmittedAt: polling.formatRelativeSubmitted,
    formatElapsedMs,
    formatMemory,
    formatHistoryTransition: dialogs.formatHistoryTransition,
    canViewSource: dialogs.canViewSource,
    canRejudgeSubmission: actions.canRejudgeSubmission,
    isRejudgingSubmission: actions.isRejudgingSubmission,
    applySubmissionFilters: query.applySubmissionFilters,
    resetSubmissionFilters: query.resetSubmissionFilters,
    openHistoryDialog: dialogs.openHistoryDialog,
    closeHistoryDialog: dialogs.closeHistoryDialog,
    openSourceDialog: dialogs.openSourceDialog,
    closeSourceDialog: dialogs.closeSourceDialog,
    copySourceCode: dialogs.copySourceCode,
    handleRejudgeSubmission: actions.handleRejudgeSubmission,
    refreshSubmissions,
    goToPreviousPage,
    goToNextPage
  }
}
