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
  const countFormatter = new Intl.NumberFormat()

  const formatCount = (value) => countFormatter.format(value)
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

  let listResource

  const pagination = useSubmissionPagination({
    submissions: computed(() => listResource?.submissions.value ?? []),
    formatCount,
    loadSubmissions(options){
      if (!listResource) {
        return
      }

      if (
        listResource.isLoading.value &&
        (
          options?.beforeSubmissionId === pagination.currentBeforeSubmissionId.value ||
          (!options?.beforeSubmissionId && !pagination.currentBeforeSubmissionId.value)
        )
      ) {
        return
      }

      listResource.loadSubmissions(options)
    }
  })

  let actions

  listResource = useSubmissionListResource({
    isAuthenticated,
    authenticatedBearerToken,
    isMineScope: query.isMineScope,
    routeState: query.routeState,
    numericProblemId: query.numericProblemId,
    currentUserId,
    appliedLanguageFilter: query.appliedLanguageFilter,
    selectedLanguageFilter: query.selectedLanguageFilter,
    pagination,
    resetRejudgingSubmissions(){
      actions?.resetRejudgingSubmissions()
    }
  })

  actions = useSubmissionActions({
    authState,
    formatCount,
    patchSubmission: listResource.patchSubmission
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

    void listResource.loadSubmissions()
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
      void listResource.loadSubmissions()
    }
  )

  function goToPreviousPage(){
    if (listResource.isLoading.value || !pagination.hasPreviousPage.value) {
      return
    }

    pagination.goToPreviousPage()
  }

  function goToNextPage(){
    if (
      listResource.isLoading.value ||
      !pagination.hasMoreSubmissions.value ||
      !Number.isInteger(pagination.nextBeforeSubmissionId.value)
    ) {
      return
    }

    pagination.goToNextPage()
  }

  onMounted(async () => {
    query.syncSelectedFiltersFromRoute()
    void listResource.loadSupportedSubmissionLanguages()

    if (!authState.initialized) {
      await initializeAuth()
    }

    if (!listResource.hasLoadedOnce.value) {
      void listResource.loadSubmissions()
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
    formatElapsedMs: (value) => formatElapsedMs(countFormatter, value),
    formatMemory: (value) => formatMemory(countFormatter, value),
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
    refreshSubmissions: listResource.refreshSubmissions,
    goToPreviousPage,
    goToNextPage
  }
}
